package com.arenax3;

import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.FileProvider;
import androidx.work.Constraints;
import androidx.work.NetworkType;
import androidx.work.PeriodicWorkRequest;
import androidx.work.WorkManager;
import androidx.work.Worker;
import androidx.work.WorkerParameters;
import com.android.volley.Request;
import com.android.volley.toolbox.JsonObjectRequest;
import com.android.volley.toolbox.Volley;
import org.json.JSONException;
import java.io.File;
import java.util.concurrent.TimeUnit;

public class UpdatesActivity extends AppCompatActivity {

    private TextView textViewCurrentVersion;
    private TextView textViewLatestVersion;
    private TextView textViewChangelog;
    private TextView textViewUpdateStatus;
    private Button buttonCheckUpdates;
    private Button buttonDownloadUpdate;
    private Button buttonInstallUpdate;
    private ProgressBar progressBar;
    private ProgressBar progressBarDownload;

    private String latestVersion = "";
    private String downloadUrl = "";
    private String changelog = "";
    private long downloadId = -1;
    private SharedPreferences sharedPreferences;
    private WorkManager workManager;

    private static final String UPDATE_CHECK_URL = "https://api.arenax3.com/updates/latest.json";
    private static final String PREFS_NAME = "ArenaX3Updates";
    private static final String DOWNLOAD_FOLDER = "ArenaX3/Updates/";
    private static final String UPDATE_WORK_NAME = "arenax3_update_check";

    private final BroadcastReceiver downloadReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, -1);
            if (id == downloadId) {
                progressBarDownload.setVisibility(android.view.View.GONE);
                buttonInstallUpdate.setEnabled(true);
                buttonDownloadUpdate.setEnabled(false);
                textViewUpdateStatus.setText("Download complete! Tap Install to update.");
                Toast.makeText(UpdatesActivity.this, "Update downloaded", Toast.LENGTH_LONG).show();
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_updates);

        initializeViews();
        workManager = WorkManager.getInstance(this);
        sharedPreferences = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);

        setupClickListeners();
        registerDownloadReceiver();
        displayCurrentVersion();
        schedulePeriodicUpdateCheck();
    }

    private void initializeViews() {
        textViewCurrentVersion = findViewById(R.id.text_view_current_version);
        textViewLatestVersion = findViewById(R.id.text_view_latest_version);
        textViewChangelog = findViewById(R.id.text_view_changelog);
        textViewUpdateStatus = findViewById(R.id.text_view_update_status);
        buttonCheckUpdates = findViewById(R.id.button_check_updates);
        buttonDownloadUpdate = findViewById(R.id.button_download_update);
        buttonInstallUpdate = findViewById(R.id.button_install_update);
        progressBar = findViewById(R.id.progress_bar);
        progressBarDownload = findViewById(R.id.progress_bar_download);

        buttonDownloadUpdate.setEnabled(false);
        buttonInstallUpdate.setEnabled(false);
    }

    private void setupClickListeners() {
        buttonCheckUpdates.setOnClickListener(v -> checkForUpdates());
        buttonDownloadUpdate.setOnClickListener(v -> downloadUpdate());
        buttonInstallUpdate.setOnClickListener(v -> installUpdate());
    }

    private void registerDownloadReceiver() {
        registerReceiver(downloadReceiver, new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));
    }

    private void displayCurrentVersion() {
        try {
            PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            textViewCurrentVersion.setText("Current: v" + packageInfo.versionName);
        } catch (PackageManager.NameNotFoundException e) {
            textViewCurrentVersion.setText("Current: Unknown");
        }
    }

    private void schedulePeriodicUpdateCheck() {
        Constraints constraints = new Constraints.Builder()
            .setRequiredNetworkType(NetworkType.CONNECTED)
            .build();

        PeriodicWorkRequest updateWorkRequest = new PeriodicWorkRequest.Builder(
            UpdateCheckWorker.class, 24, TimeUnit.HOURS)
            .setConstraints(constraints)
            .build();

        workManager.enqueueUniquePeriodicWork(
            UPDATE_WORK_NAME,
            androidx.work.ExistingPeriodicWorkPolicy.KEEP,
            updateWorkRequest
        );
    }

    private void checkForUpdates() {
        progressBar.setVisibility(android.view.View.VISIBLE);
        buttonCheckUpdates.setEnabled(false);
        textViewUpdateStatus.setText("Checking for updates...");

        JsonObjectRequest request = new JsonObjectRequest(Request.Method.GET, UPDATE_CHECK_URL, null,
            response -> {
                progressBar.setVisibility(android.view.View.GONE);
                buttonCheckUpdates.setEnabled(true);
                
                try {
                    latestVersion = response.getString("version");
                    downloadUrl = response.getString("download_url");
                    changelog = response.getString("changelog");
                    
                    textViewLatestVersion.setText("Latest: v" + latestVersion);
                    textViewChangelog.setText(changelog);
                    
                    sharedPreferences.edit()
                        .putString("latest_version", latestVersion)
                        .putLong("last_check", System.currentTimeMillis())
                        .apply();
                    
                    compareVersions(latestVersion);
                    
                } catch (JSONException e) {
                    textViewUpdateStatus.setText("Error parsing update info");
                }
            },
            error -> {
                progressBar.setVisibility(android.view.View.GONE);
                buttonCheckUpdates.setEnabled(true);
                textViewUpdateStatus.setText("Failed to check updates");
            });
        
        Volley.newRequestQueue(this).add(request);
    }

    private void compareVersions(String latest) {
        try {
            String currentVersion = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
            
            if (!currentVersion.equals(latest)) {
                textViewUpdateStatus.setText("Update available! v" + latest);
                buttonDownloadUpdate.setEnabled(true);
                showUpdateDialog();
            } else {
                textViewUpdateStatus.setText("You are running the latest version!");
                buttonDownloadUpdate.setEnabled(false);
            }
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
    }

    private void showUpdateDialog() {
        new AlertDialog.Builder(this)
            .setTitle("Update Available")
            .setMessage("Version " + latestVersion + " is available.\n\n" + changelog)
            .setPositiveButton("Download", (dialog, which) -> downloadUpdate())
            .setNegativeButton("Later", null)
            .show();
    }

    private void downloadUpdate() {
        if (downloadUrl.isEmpty()) {
            Toast.makeText(this, "Download URL not available", Toast.LENGTH_SHORT).show();
            return;
        }

        File downloadDir = new File(Environment.getExternalStorageDirectory(), DOWNLOAD_FOLDER);
        if (!downloadDir.exists()) downloadDir.mkdirs();

        String fileName = "ArenaX3_v" + latestVersion + ".apk";
        File destinationFile = new File(downloadDir, fileName);
        if (destinationFile.exists()) destinationFile.delete();

        DownloadManager.Request request = new DownloadManager.Request(Uri.parse(downloadUrl));
        request.setTitle("ArenaX3 Update");
        request.setDescription("Downloading version " + latestVersion);
        request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        request.setDestinationUri(Uri.fromFile(destinationFile));
        request.setMimeType("application/vnd.android.package-archive");

        DownloadManager downloadManager = (DownloadManager) getSystemService(DOWNLOAD_SERVICE);
        downloadId = downloadManager.enqueue(request);

        progressBarDownload.setVisibility(android.view.View.VISIBLE);
        buttonDownloadUpdate.setEnabled(false);
        textViewUpdateStatus.setText("Downloading update...");
    }

    private void installUpdate() {
        File downloadDir = new File(Environment.getExternalStorageDirectory(), DOWNLOAD_FOLDER);
        String fileName = "ArenaX3_v" + latestVersion + ".apk";
        File updateFile = new File(downloadDir, fileName);

        if (!updateFile.exists()) {
            Toast.makeText(this, "Update file not found", Toast.LENGTH_SHORT).show();
            return;
        }

        Intent intent = new Intent(Intent.ACTION_VIEW);
        Uri uri = FileProvider.getUriForFile(this, getPackageName() + ".fileprovider", updateFile);
        intent.setDataAndType(uri, "application/vnd.android.package-archive");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            unregisterReceiver(downloadReceiver);
        } catch (IllegalArgumentException ignored) {}
    }

    public static class UpdateCheckWorker extends Worker {
        public UpdateCheckWorker(@NonNull Context context, @NonNull WorkerParameters params) {
            super(context, params);
        }

        @NonNull
        @Override
        public Result doWork() {
            return Result.success();
        }
    }
}