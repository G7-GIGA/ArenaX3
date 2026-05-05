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
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class GamesActivity extends AppCompatActivity {

    private ListView listViewGames;
    private TextView textViewNoGames;
    private Button buttonBrowseGames;
    private Button buttonLoadRecent;
    private List<String> gamePaths;
    private List<String> gameNames;
    private ArrayAdapter<String> adapter;
    private static final String GAMES_DIRECTORY = Environment.getExternalStorageDirectory() + "/ArenaX3/Games/";
    private ActivityResultLauncher<String> requestPermissionLauncher;
    private ActivityResultLauncher<String[]> requestMultiplePermissionsLauncher;
    private ActivityResultLauncher<Intent> browseGameLauncher;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_games);

        initializeViews();
        setupPermissionLaunchers();
        setupBrowseLauncher();
        checkPermissionsAndLoadGames();
        setupClickListeners();
    }

    private void initializeViews() {
        listViewGames = findViewById(R.id.list_view_games);
        textViewNoGames = findViewById(R.id.text_view_no_games);
        buttonBrowseGames = findViewById(R.id.button_browse_games);
        buttonLoadRecent = findViewById(R.id.button_load_recent);

        gamePaths = new ArrayList<>();
        gameNames = new ArrayList<>();
        adapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, gameNames);
        listViewGames.setAdapter(adapter);
    }

    private void setupPermissionLaunchers() {
        requestPermissionLauncher = registerForActivityResult(
            new ActivityResultContracts.RequestPermission(),
            isGranted -> {
                if (isGranted) {
                    loadGamesFromDirectory();
                } else {
                    showPermissionDeniedDialog();
                }
            }
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            requestMultiplePermissionsLauncher = registerForActivityResult(
                new ActivityResultContracts.RequestMultiplePermissions(),
                result -> {
                    boolean allGranted = true;
                    for (boolean granted : result.values()) {
                        if (!granted) allGranted = false;
                    }
                    if (allGranted) {
                        loadGamesFromDirectory();
                    } else {
                        showPermissionDeniedDialog();
                    }
                }
            );
        }
    }

    private void setupBrowseLauncher() {
        browseGameLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        loadGameFromUri(uri);
                    }
                }
            }
        );
    }

    private void checkPermissionsAndLoadGames() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (Environment.isExternalStorageManager()) {
                loadGamesFromDirectory();
            } else {
                requestManageStoragePermission();
            }
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            String[] permissions = {
                Manifest.permission.READ_MEDIA_AUDIO,
                Manifest.permission.READ_MEDIA_VIDEO,
                Manifest.permission.READ_MEDIA_IMAGES
            };
            boolean allGranted = true;
            for (String perm : permissions) {
                if (ContextCompat.checkSelfPermission(this, perm) != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }
            if (allGranted) {
                loadGamesFromDirectory();
            } else {
                requestMultiplePermissionsLauncher.launch(permissions);
            }
        } else {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                requestPermissionLauncher.launch(Manifest.permission.READ_EXTERNAL_STORAGE);
            } else {
                loadGamesFromDirectory();
            }
        }
    }

    private void requestManageStoragePermission() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Storage Permission Required");
        builder.setMessage("ArenaX3 needs All Files Access to browse games from storage.");
        builder.setPositiveButton("Grant", (dialog, which) -> {
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.setData(Uri.parse("package:" + getPackageName()));
            startActivity(intent);
        });
        builder.setNegativeButton("Cancel", (dialog, which) -> {
            Toast.makeText(this, "Cannot browse games without permission", Toast.LENGTH_SHORT).show();
            finish();
        });
        builder.show();
    }

    private void showPermissionDeniedDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Permission Required");
        builder.setMessage("Storage permission is required to browse and load games.");
        builder.setPositiveButton("Retry", (dialog, which) -> checkPermissionsAndLoadGames());
        builder.setNegativeButton("Exit", (dialog, which) -> finish());
        builder.show();
    }

    private void loadGamesFromDirectory() {
        File gamesDir = new File(GAMES_DIRECTORY);
        if (!gamesDir.exists()) {
            gamesDir.mkdirs();
            textViewNoGames.setVisibility(View.VISIBLE);
            listViewGames.setVisibility(View.GONE);
            return;
        }

        gamePaths.clear();
        gameNames.clear();

        File[] files = gamesDir.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.isFile() && isGameFile(file.getName())) {
                    gamePaths.add(file.getAbsolutePath());
                    gameNames.add(file.getName());
                }
            }
        }

        if (gameNames.isEmpty()) {
            textViewNoGames.setVisibility(View.VISIBLE);
            listViewGames.setVisibility(View.GONE);
        } else {
            textViewNoGames.setVisibility(View.GONE);
            listViewGames.setVisibility(View.VISIBLE);
            adapter.notifyDataSetChanged();
        }
    }

    private boolean isGameFile(String fileName) {
        String lower = fileName.toLowerCase();
        return lower.endsWith(".bin") || lower.endsWith(".cue") || lower.endsWith(".iso") ||
               lower.endsWith(".img") || lower.endsWith(".chd") || lower.endsWith(".ecm") ||
               lower.endsWith(".pbp") || lower.endsWith(".zip") || lower.endsWith(".7z");
    }

    private void loadGameFromUri(Uri uri) {
        String path = uri.getPath();
        if (path != null) {
            Toast.makeText(this, "Loading game: " + uri.getLastPathSegment(), Toast.LENGTH_SHORT).show();
            Intent resultIntent = new Intent();
            resultIntent.setData(uri);
            setResult(RESULT_OK, resultIntent);
            finish();
        }
    }

    private void setupClickListeners() {
        listViewGames.setOnItemClickListener((parent, view, position, id) -> {
            String gamePath = gamePaths.get(position);
            Toast.makeText(this, "Loading: " + gameNames.get(position), Toast.LENGTH_SHORT).show();
            Intent resultIntent = new Intent();
            resultIntent.putExtra("GAME_PATH", gamePath);
            resultIntent.putExtra("GAME_NAME", gameNames.get(position));
            setResult(RESULT_OK, resultIntent);
            finish();
        });

        buttonBrowseGames.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            String[] mimeTypes = {"application/octet-stream", "application/x-cue", "application/x-iso"};
            intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
            browseGameLauncher.launch(intent);
        });

        buttonLoadRecent.setOnClickListener(v -> {
            Toast.makeText(this, "Recent games feature coming soon", Toast.LENGTH_SHORT).show();
        });
    }
}