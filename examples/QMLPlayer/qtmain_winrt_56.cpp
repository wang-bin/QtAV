/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Windows main function of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  This file contains the code in the qtmain library for WinRT.
  qtmain contains the WinRT startup code and is required for
  linking to the Qt DLL.

  When a Windows application starts, the WinMain function is
  invoked. This WinMain creates the WinRT application
  container, which in turn calls the application's main()
  entry point within the newly created GUI thread.
*/

#if _MSC_VER < 1900
#include <new.h>

typedef struct
{
    int newmode;
} _startupinfo;
#endif // _MSC_VER < 1900

extern "C" {
#if _MSC_VER < 1900
    int __getmainargs(int *argc, char ***argv, char ***env, int expandWildcards, _startupinfo *info);
#endif
    int main(int, char **);
}

#include <qbytearray.h>
#include <qstring.h>
#include <qdir.h>
#include <qstandardpaths.h>
#include <qfunctions_winrt.h>
#include <qcoreapplication.h>

#include <wrl.h>
#include <Windows.ApplicationModel.core.h>
#include <windows.ui.xaml.h>
#include <windows.ui.xaml.controls.h>

using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

static QString UrlFromFileArgs(IFileActivatedEventArgs *fileArgs);

#define qHString(x) Wrappers::HString::MakeReference(x).Get()
#define CoreApplicationClass RuntimeClass_Windows_ApplicationModel_Core_CoreApplication
typedef ITypedEventHandler<CoreApplicationView *, Activation::IActivatedEventArgs *> ActivatedHandler;

static QtMessageHandler defaultMessageHandler;
static void devMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
#ifndef Q_OS_WINPHONE
    static HANDLE shmem = 0;
    static HANDLE event = 0;
    if (!shmem)
        shmem = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 4096, L"qdebug-shmem");
    if (!event)
        event = CreateEventEx(NULL, L"qdebug-event", 0, EVENT_ALL_ACCESS);

    void *data = MapViewOfFileFromApp(shmem, FILE_MAP_WRITE, 0, 4096);
    memset(data, quint32(type), sizeof(quint32));
    memcpy_s(static_cast<quint32 *>(data) + 1, 4096 - sizeof(quint32),
             message.data(), (message.length() + 1) * sizeof(wchar_t));
    UnmapViewOfFile(data);
    SetEvent(event);
#endif // !Q_OS_WINPHONE
    defaultMessageHandler(type, context, message);
}

class QActivationEvent : public QEvent
{
public:
    explicit QActivationEvent(IInspectable *args)
        : QEvent(QEvent::WinEventAct)
    {
        setAccepted(false);
        args->AddRef();
        d = reinterpret_cast<QEventPrivate *>(args);
    }

    ~QActivationEvent() {
        IUnknown *args = reinterpret_cast<IUnknown *>(d);
        args->Release();
        d = nullptr;
    }
};

class AppContainer : public RuntimeClass<Xaml::IApplicationOverrides>
{
public:
    AppContainer()
    {
        ComPtr<Xaml::IApplicationFactory> applicationFactory;
        HRESULT hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Application).Get(),
                                            IID_PPV_ARGS(&applicationFactory));
        Q_ASSERT_SUCCEEDED(hr);

        hr = applicationFactory->CreateInstance(this, &base, &core);
        RETURN_VOID_IF_FAILED("Failed to create application container instance");

        pidFile = INVALID_HANDLE_VALUE;
    }

    ~AppContainer()
    {
    }

    int exec(int argc, char **argv)
    {
        qDebug("qtmain.exec");
        args.reserve(argc);
        for (int i = 0; i < argc; ++i)
            args.append(argv[i]);

        mainThread = CreateThread(NULL, 0, [](void *param) -> DWORD {
            AppContainer *app = reinterpret_cast<AppContainer *>(param);
            int argc = app->args.count();
            char **argv = app->args.data();
            const int res = main(argc, argv);
            if (app->pidFile != INVALID_HANDLE_VALUE) {
                const QByteArray resString = QByteArray::number(res);
                WriteFile(app->pidFile, reinterpret_cast<LPCVOID>(resString.constData()),
                          resString.size(), NULL, NULL);
                FlushFileBuffers(app->pidFile);
                CloseHandle(app->pidFile);
            }
            app->core->Exit();
            return res;
        }, this, CREATE_SUSPENDED, nullptr);

        qDebug("qtmain.exec RoGetActivationFactory. mainthread: %p", mainThread);
        HRESULT hr;
        ComPtr<Xaml::IApplicationStatics> appStatics;
        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Application).Get(),
                                    IID_PPV_ARGS(&appStatics));
        qDebug("qtmain.exec start");
        Q_ASSERT_SUCCEEDED(hr);
        hr = appStatics->Start(Callback<Xaml::IApplicationInitializationCallback>([](Xaml::IApplicationInitializationCallbackParams *) {
            return S_OK;
        }).Get());
        Q_ASSERT_SUCCEEDED(hr);

        WaitForSingleObjectEx(mainThread, INFINITE, FALSE);
        DWORD exitCode;
        GetExitCodeThread(mainThread, &exitCode);
        return exitCode;
    }

private:
    HRESULT __stdcall OnActivated(IActivatedEventArgs *args) Q_DECL_OVERRIDE
    {
        QAbstractEventDispatcher *dispatcher = QCoreApplication::eventDispatcher();
        qDebug("qtmain.OnActivated. dispatcher: ", dispatcher);
        if (dispatcher)
            QCoreApplication::postEvent(dispatcher, new QActivationEvent(args));
        return S_OK;
    }

    HRESULT __stdcall OnLaunched(ILaunchActivatedEventArgs *launchArgs) Q_DECL_OVERRIDE
    {
        qDebug("qtmain.OnLaunched");
#if _MSC_VER >= 1900
        ComPtr<IPrelaunchActivatedEventArgs> preArgs;
        HRESULT hr = launchArgs->QueryInterface(preArgs.GetAddressOf());
        if (SUCCEEDED(hr)) {
            boolean prelaunched;
            preArgs->get_PrelaunchActivated(&prelaunched);
            if (prelaunched)
                return S_OK;
        }
        commandLine = QString::fromWCharArray(GetCommandLine()).toUtf8();
#endif
        HString launchCommandLine;
        launchArgs->get_Arguments(launchCommandLine.GetAddressOf());
        if (launchCommandLine.IsValid()) {
            quint32 launchCommandLineLength;
            const wchar_t *launchCommandLineBuffer = launchCommandLine.GetRawBuffer(&launchCommandLineLength);
            if (!commandLine.isEmpty() && launchCommandLineLength)
                commandLine += ' ';
            if (launchCommandLineLength)
                commandLine += QString::fromWCharArray(launchCommandLineBuffer, launchCommandLineLength).toUtf8();
        }
        if (!commandLine.isEmpty())
            args.append(commandLine.data());

        bool quote = false;
        bool escape = false;
        for (int i = 0; i < commandLine.size(); ++i) {
            switch (commandLine.at(i)) {
            case '\\':
                escape = true;
                break;
            case '"':
                if (escape) {
                    escape = false;
                    break;
                }
                quote = !quote;
                commandLine[i] = '\0';
                break;
            case ' ':
                if (quote)
                    break;
                commandLine[i] = '\0';
                if (args.last()[0] != '\0')
                    args.append(commandLine.data() + i + 1);
                // fall through
            default:
                if (args.last()[0] == '\0')
                    args.last() = commandLine.data() + i;
                escape = false; // only quotes are escaped
                break;
            }
        }

        if (args.count() >= 2 && strncmp(args.at(1), "-ServerName:", 12) == 0)
            args.remove(1);

        bool develMode = false;
        bool debugWait = false;
        foreach (const char *arg, args) {
            if (strcmp(arg, "-qdevel") == 0)
                develMode = true;
            if (strcmp(arg, "-qdebug") == 0)
                debugWait = true;
        }
        if (develMode) {
            // Write a PID file to help runner
            const QString pidFileName = QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
                    .absoluteFilePath(QString::number(uint(GetCurrentProcessId())) + QStringLiteral(".pid"));
            CREATEFILE2_EXTENDED_PARAMETERS params = {
                sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
                FILE_ATTRIBUTE_NORMAL
            };
            pidFile = CreateFile2(reinterpret_cast<LPCWSTR>(pidFileName.utf16()),
                        GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, &params);
            // Install the develMode message handler
#ifndef Q_OS_WINPHONE
            defaultMessageHandler = qInstallMessageHandler(devMessageHandler);
#endif
        }
        // Wait for debugger before continuing
        if (debugWait) {
            while (!IsDebuggerPresent())
                WaitForSingleObjectEx(GetCurrentThread(), 1, true);
        }

        ResumeThread(mainThread);
        return S_OK;
    }

    HRESULT __stdcall OnFileActivated(IFileActivatedEventArgs *args) Q_DECL_OVERRIDE
    {
        qDebug("qtmain.OnFileActivated");
        Q_UNUSED(args);
        const QString file = UrlFromFileArgs(args);
        this->args.append("-f");
        this->args.append(strdup(file.toLocal8Bit().constData()));
        ResumeThread(mainThread);
        QAbstractEventDispatcher *dispatcher = QCoreApplication::eventDispatcher();
        qDebug("qtmain.OnFileActivated. dispatcher: ", dispatcher);
        if (dispatcher)
            QCoreApplication::postEvent(dispatcher, new QActivationEvent(args));
        return S_OK;
    }

    HRESULT __stdcall OnSearchActivated(ISearchActivatedEventArgs *args) Q_DECL_OVERRIDE
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnShareTargetActivated(IShareTargetActivatedEventArgs *args) Q_DECL_OVERRIDE
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnFileOpenPickerActivated(IFileOpenPickerActivatedEventArgs *args) Q_DECL_OVERRIDE
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnFileSavePickerActivated(IFileSavePickerActivatedEventArgs *args) Q_DECL_OVERRIDE
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnCachedFileUpdaterActivated(ICachedFileUpdaterActivatedEventArgs *args) Q_DECL_OVERRIDE
    {
        Q_UNUSED(args);
        return S_OK;
    }

    HRESULT __stdcall OnWindowCreated(Xaml::IWindowCreatedEventArgs *args) Q_DECL_OVERRIDE
    {
        Q_UNUSED(args);
        return S_OK;
    }

    ComPtr<Xaml::IApplicationOverrides> base;
    ComPtr<Xaml::IApplication> core;
    QByteArray commandLine;
    QVarLengthArray<char *> args;
    HANDLE mainThread;
    HANDLE pidFile;
};

// Main entry point for Appx containers
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    int argc = 0;
    char **argv = 0, **env = 0;
#if _MSC_VER < 1900
    _startupinfo info = { _query_new_mode() };
    if (int init = __getmainargs(&argc, &argv, &env, false, &info))
        return init;
#endif // _MSC_VER >= 1900
    for (int i = 0; env && env[i]; ++i) {
        QByteArray var(env[i]);
        int split = var.indexOf('=');
        if (split > 0)
            qputenv(var.mid(0, split), var.mid(split + 1));
    }

    if (FAILED(RoInitialize(RO_INIT_MULTITHREADED)))
        return 1;

    ComPtr<AppContainer> app = Make<AppContainer>();
    return app->exec(argc, argv);
}


#include <windows.foundation.h>
#include <windows.storage.pickers.h>
#include <Windows.ApplicationModel.activation.h>
#include <qfunctions_winrt.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Pickers;
#include <QtDebug>

#define COM_LOG_COMPONENT "WinRT"
#define COM_ENSURE(f, ...) COM_CHECK(f, return __VA_ARGS__;)
#define COM_WARN(f) COM_CHECK(f)
#define COM_CHECK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            qWarning() << QString::fromLatin1(COM_LOG_COMPONENT " error@%1. " #f ": (0x%2) %3").arg(__LINE__).arg(hr, 0, 16).arg(qt_error_string(hr)); \
            __VA_ARGS__ \
        } \
    } while (0)

QString UrlFromFileArgs(IFileActivatedEventArgs *fileArgs)
{
    ComPtr<IVectorView<IStorageItem*>> files;
    COM_ENSURE(fileArgs->get_Files(&files), QString());
    ComPtr<IStorageItem> item;
    COM_ENSURE(files->GetAt(0, &item), QString());
    HString path;
    COM_ENSURE(item->get_Path(path.GetAddressOf()), QString());

    quint32 pathLen;
    const wchar_t *pathStr = path.GetRawBuffer(&pathLen);
    const QString filePath = QString::fromWCharArray(pathStr, pathLen);
    qDebug() << "file path: " << filePath;
    item->AddRef(); //ensure we can access it later. TODO: how to release?
    return QString::fromLatin1("winrt:@%1:%2").arg((qint64)(qptrdiff)item.Get()).arg(filePath);
}
