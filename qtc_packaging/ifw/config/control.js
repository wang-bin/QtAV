//https://doc.qt.io/qtinstallerframework/noninteractive.html
//https://doc.qt.io/qtinstallerframework/scripting-installer.html
function Controller() 
{
    //installer.installationFinished.connect(onInstalled)
    installer.uninstallationFinished.connect(function(){
      console.log("uninstall done")
      if (installer.value("os") === "win") {
          var unreg = "delete;HKEY_CURRENT_USER\\SOFTWARE\\Classes\\*\\shell\\Open with QtAV;/f"
          var unreg2 = "delete;HKEY_CURRENT_USER\\SOFTWARE\\Classes\\*\\shell\\Open with QtAV QML;/f"
          installer.executeDetached("reg", unreg.split(";"))
          installer.executeDetached("reg", unreg2.split(";"))
          //QMessageBox.information("", "finish", "unreg: " + unreg2.split(";"))
      }
    })
}

Controller.prototype.LicenseAgreementPageCallback = function()
{
    var w = gui.currentPageWidget()
    if (w != null)
        w.AcceptLicenseRadioButton.checked = true
}
/*
Controller.prototype.PerformInstallationPageCallback() {
    if (installer.isInstaller()) {
    }
    if (installer.isUninstaller()) {
    }
    var page = gui.currentPageWidget()
    if (!page)
        return
}
*/
