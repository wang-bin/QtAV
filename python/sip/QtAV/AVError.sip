namespace QtAV {

class AVError
{
%TypeHeaderCode
#include <QtAV/AVError.h>
%End

public:
    enum ErrorCode {
        NoError,

        //open/read/seek network stream error. value must be less then ResourceError because of correct_error_by_ffmpeg
        NetworkError, // all above and before NoError are NetworkError

        OpenTimedout,
        OpenError,
        ParseStreamTimedOut,
        FindStreamInfoTimedout = ParseStreamTimedOut,
        ParseStreamError,
        FindStreamInfoError = ParseStreamError,
        StreamNotFound, //a,v,s?
        ReadTimedout,
        ReadError,
        SeekError,
        ResourceError, // all above and before NetworkError are ResourceError

        OpenCodecError,
        CloseCodecError,
        AudioCodecNotFound,
        VideoCodecNotFound,
        SubtitleCodecNotFound,
        CodecError, // all above and before NoError are CodecError

        FormatError, // all above and before CodecError are FormatError

        // decrypt error. Not implemented
        AccessDenied, // all above and before NetworkError are AccessDenied

        UnknowError
    };

    AVError();
    AVError(ErrorCode code, int ffmpegError = 0);
    AVError(ErrorCode code, const QString& detail, int ffmpegError = 0);
    AVError(const AVError& other);

    //AVError &operator=(const AVError &other);
    bool operator==(const AVError &other) const;
    bool operator!=(const AVError &other) const;

    void setError(ErrorCode ec);
    ErrorCode error() const;
    QString string() const;

    int ffmpegErrorCode() const;
    QString ffmpegErrorString() const;
};

};
