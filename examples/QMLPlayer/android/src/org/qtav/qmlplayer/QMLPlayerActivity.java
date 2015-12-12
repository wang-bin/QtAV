package org.qtav.qmlplayer;
import org.qtproject.qt5.android.bindings.QtActivity;
import android.content.Context;
import android.content.Intent;
import android.app.PendingIntent;
import android.util.Log;
import android.os.Bundle;
import android.view.WindowManager;
import android.content.pm.ActivityInfo;

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
}
