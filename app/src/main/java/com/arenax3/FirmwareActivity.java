package com.arenax3;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class FirmwareActivity extends AppCompatActivity {

    private TextView textViewCurrentFirmware;
    private TextView textViewFirmwareStatus;
    private TextView textViewFirmwarePath;
    private Button buttonSelectFirmware;
    private Button buttonVerifyFirmware;
    private Button buttonInstallFirmware;
    private ProgressBar progressBar;
    private CheckBox checkBoxConfirmInstall;
    private String selectedFirmwarePath = null;
    private static final String FIRMWARE_DIR = Environment.getExternalStorageDirectory() + "/ArenaX3/Bios/";
    private static final String FIRMWARE_DEST = Environment.getExternalStorageDirectory() + "/ArenaX3/bios.bin";
    private static final String EXPECTED_MD5_BIOS = "dcb8c7f6e8f2a4b9c1d3e5f7a9b2c4d6";
    private static final String EXPECTED_MD5_FIRMWARE = "9a3b5c7d9e1f3a5b7c9d1e3f5a7b9c1d";

    private ActivityResultLauncher<String> requestPermissionLauncher;
    private ActivityResultLauncher<Intent> selectFirmwareLauncher;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_firmware);

        initializeViews();
        setupPermissionLaunchers();
        setupFileLauncher();
        checkPermissionsAndLoadFirmware();
        setupClickListeners();
    }

    private void initializeViews() {
        textViewCurrentFirmware = findViewById(R.id.text_view_current_firmware);
        textViewFirmwareStatus = findViewById(R.id.text_view_firmware_status);
        textViewFirmwarePath = findViewById(R.id.text_view_firmware_path);
        buttonSelectFirmware = findViewById(R.id.button_select_firmware);
        buttonVerifyFirmware = findViewById(R.id.button_verify_firmware);
        buttonInstallFirmware = findViewById(R.id.button_install_firmware);
        progressBar = findViewById(R.id.progress_bar_firmware);
        checkBoxConfirmInstall = findViewById(R.id.check_box_confirm_install);
    }

    private void setupPermissionLaunchers() {
        requestPermissionLauncher = registerForActivityResult(
            new ActivityResultContracts.RequestPermission(),
            isGranted -> {
                if (isGranted) {
                    loadCurrentFirmware();
                } else {
                    showPermissionDeniedDialog();
                }
            }
        );
    }

    private void setupFileLauncher() {
        selectFirmwareLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        selectedFirmwarePath = uri.getPath();
                        textViewFirmwarePath.setText("Selected: " + uri.getLastPathSegment());
                        buttonVerifyFirmware.setEnabled(true);
                        textViewFirmwareStatus.setText("Firmware selected, ready for verification");
                        textViewFirmwareStatus.setTextColor(getColor(android.R.color.holo_orange_dark));
                    }
                }
            }
        );
    }

    private void checkPermissionsAndLoadFirmware() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (Environment.isExternalStorageManager()) {
                loadCurrentFirmware();
            } else {
                requestManageStoragePermission();
            }
        } else {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                requestPermissionLauncher.launch(Manifest.permission.READ_EXTERNAL_STORAGE);
            } else {
                loadCurrentFirmware();
            }
        }
    }

    private void requestManageStoragePermission() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Storage Permission Required");
        builder.setMessage("ArenaX3 needs storage access to load and verify BIOS/firmware files.");
        builder.setPositiveButton("Grant", (dialog, which) -> {
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.setData(Uri.parse("package:" + getPackageName()));
            startActivity(intent);
        });
        builder.setNegativeButton("Cancel", (dialog, which) -> {
            Toast.makeText(this, "Firmware loading cancelled", Toast.LENGTH_SHORT).show();
            finish();
        });
        builder.show();
    }

    private void showPermissionDeniedDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Permission Denied");
        builder.setMessage("Storage permission is required to load firmware files.");
        builder.setPositiveButton("Retry", (dialog, which) -> checkPermissionsAndLoadFirmware());
        builder.setNegativeButton("Exit", (dialog, which) -> finish());
        builder.show();
    }

    private void loadCurrentFirmware() {
        File firmwareFile = new File(FIRMWARE_DEST);
        if (firmwareFile.exists()) {
            textViewCurrentFirmware.setText("BIOS/Firmware: " + firmwareFile.getName());
            textViewCurrentFirmware.setTextColor(getColor(android.R.color.holo_green_dark));
            textViewFirmwareStatus.setText("Firmware is installed");
            textViewFirmwareStatus.setTextColor(getColor(android.R.color.holo_green_light));
        } else {
            textViewCurrentFirmware.setText("No firmware installed");
            textViewCurrentFirmware.setTextColor(getColor(android.R.color.holo_red_dark));
            textViewFirmwareStatus.setText("Firmware required to run games");
            textViewFirmwareStatus.setTextColor(getColor(android.R.color.holo_orange_dark));
        }
    }

    private void setupClickListeners() {
        buttonSelectFirmware.setOnClickListener(v -> selectFirmwareFile());
        buttonVerifyFirmware.setOnClickListener(v -> verifyFirmware());
        buttonInstallFirmware.setOnClickListener(v -> installFirmware());
    }

    private void selectFirmwareFile() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        String[] mimeTypes = {"application/octet-stream", "application/x-bios", "application/x-firmware"};
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        selectFirmwareLauncher.launch(intent);
    }

    private void verifyFirmware() {
        if (selectedFirmwarePath == null) {
            Toast.makeText(this, "Please select a firmware file first", Toast.LENGTH_SHORT).show();
            return;
        }

        progressBar.setVisibility(View.VISIBLE);
        buttonVerifyFirmware.setEnabled(false);

        new Thread(() -> {
            File file = new File(selectedFirmwarePath);
            if (!file.exists()) {
                runOnUiThread(() -> {
                    progressBar.setVisibility(View.GONE);
                    buttonVerifyFirmware.setEnabled(true);
                    Toast.makeText(FirmwareActivity.this, "File not found", Toast.LENGTH_SHORT).show();
                });
                return;
            }

            String md5 = calculateMD5(file);
            boolean isValid = false;
            String type = "";

            if (md5 != null) {
                if (md5.equalsIgnoreCase(EXPECTED_MD5_BIOS)) {
                    isValid = true;
                    type = "BIOS";
                } else if (md5.equalsIgnoreCase(EXPECTED_MD5_FIRMWARE)) {
                    isValid = true;
                    type = "Firmware";
                }
            }

            final boolean finalIsValid = isValid;
            final String finalType = type;
            final String finalMd5 = md5;

            runOnUiThread(() -> {
                progressBar.setVisibility(View.GONE);
                buttonVerifyFirmware.setEnabled(true);

                if (finalIsValid) {
                    textViewFirmwareStatus.setText("✓ Valid " + finalType + " file detected");
                    textViewFirmwareStatus.setTextColor(getColor(android.R.color.holo_green_light));
                    buttonInstallFirmware.setEnabled(true);
                    Toast.makeText(FirmwareActivity.this, "Valid " + finalType + " file!", Toast.LENGTH_SHORT).show();
                } else {
                    textViewFirmwareStatus.setText("✗ Invalid or unsupported firmware. MD5: " + finalMd5);
                    textViewFirmwareStatus.setTextColor(getColor(android.R.color.holo_red_light));
                    buttonInstallFirmware.setEnabled(false);
                    Toast.makeText(FirmwareActivity.this, "Invalid firmware file", Toast.LENGTH_SHORT).show();
                }
            });
        }).start();
    }

    private void installFirmware() {
        if (!checkBoxConfirmInstall.isChecked()) {
            Toast.makeText(this, "Please confirm firmware installation", Toast.LENGTH_SHORT).show();
            return;
        }

        if (selectedFirmwarePath == null) {
            Toast.makeText(this, "No firmware selected", Toast.LENGTH_SHORT).show();
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Install Firmware");
        builder.setMessage("Installing firmware may affect emulation. Continue?");
        builder.setPositiveButton("Install", (dialog, which) -> performInstallation());
        builder.setNegativeButton("Cancel", null);
        builder.show();
    }

    private void performInstallation() {
        progressBar.setVisibility(View.VISIBLE);
        buttonInstallFirmware.setEnabled(false);
        buttonSelectFirmware.setEnabled(false);
        buttonVerifyFirmware.setEnabled(false);

        new Thread(() -> {
            try {
                File destDir = new File(FIRMWARE_DIR);
                if (!destDir.exists()) {
                    destDir.mkdirs();
                }

                File sourceFile = new File(selectedFirmwarePath);
                File destFile = new File(FIRMWARE_DEST);

                copyFile(sourceFile, destFile);

                runOnUiThread(() -> {
                    progressBar.setVisibility(View.GONE);
                    Toast.makeText(FirmwareActivity.this, "Firmware installed successfully!", Toast.LENGTH_LONG).show();
                    loadCurrentFirmware();
                    buttonInstallFirmware.setEnabled(false);
                    buttonSelectFirmware.setEnabled(true);
                    buttonVerifyFirmware.setEnabled(false);
                    checkBoxConfirmInstall.setChecked(false);
                    selectedFirmwarePath = null;
                    textViewFirmwarePath.setText("No firmware selected");
                });
            } catch (IOException e) {
                runOnUiThread(() -> {
                    progressBar.setVisibility(View.GONE);
                    Toast.makeText(FirmwareActivity.this, "Installation failed: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    buttonInstallFirmware.setEnabled(true);
                    buttonSelectFirmware.setEnabled(true);
                    buttonVerifyFirmware.setEnabled(true);
                });
                e.printStackTrace();
            }
        }).start();
    }

    private void copyFile(File source, File dest) throws IOException {
        try (InputStream in = new FileInputStream(source);
             OutputStream out = new FileOutputStream(dest)) {
            byte[] buffer = new byte[8192];
            int length;
            while ((length = in.read(buffer)) > 0) {
                out.write(buffer, 0, length);
            }
        }
    }

    private String calculateMD5(File file) {
        try {
            MessageDigest md = MessageDigest.getInstance("MD5");
            try (FileInputStream fis = new FileInputStream(file)) {
                byte[] buffer = new byte[8192];
                int length;
                while ((length = fis.read(buffer)) != -1) {
                    md.update(buffer, 0, length);
                }
            }
            byte[] digest = md.digest();
            StringBuilder sb = new StringBuilder();
            for (byte b : digest) {
                sb.append(String.format("%02x", b));
            }
            return sb.toString();
        } catch (NoSuchAlgorithmException | IOException e) {
            e.printStackTrace();
            return null;
        }
    }
}