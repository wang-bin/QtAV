package org.qtav.qmlplayer;
import org.qtproject.qt5.android.bindings.QtActivity;
import android.content.Context;
import android.content.Intent;
import android.app.PendingIntent;
import android.util.Log;
import android.os.Build;
import android.os.Bundle;
import android.view.WindowManager;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.support.v4.content.ContextCompat;
import android.support.v4.app.ActivityCompat;
import android.widget.Toast;

public class QMLPlayerActivity extends QtActivity
{
    private final static String TAG = "QMLPlayer";
    private static String m_request_url;
    private static QMLPlayerActivity m_instance;
    public QMLPlayerActivity() {
        m_instance = this;
    }
    public static String getUrl() {
        return m_instance.m_request_url;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (!checkPermission())
                requestPermission();
        }
        //getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        Intent intent = getIntent();
        String action = intent.getAction();

        Log.i("QMLPlayerActivity", "Action: " + action);
        Log.i("QMLPlayerActivity", "Data: " + intent.getDataString());
        if (intent.ACTION_VIEW.equals(action)) {
            m_request_url = intent.getDataString();
        }
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }

    protected boolean checkPermission() {
        int result = ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (result == PackageManager.PERMISSION_GRANTED)
            return true;
        return false;
    }
    protected void requestPermission() {
        if (ActivityCompat.shouldShowRequestPermissionRationale(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            Toast.makeText(this, "Please allow Write External Storage permission to play your local videos", Toast.LENGTH_LONG).show();
        } else {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
                requestPermissions(new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 100);
        }
    }
}
