package com.arenax3;

import android.os.Bundle;
import android.view.View;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private CardView cardGames;
    private CardView cardFirmware;
    private CardView cardControls;
    private CardView cardSettings;
    private CardView cardUpdates;
    private CardView cardAbout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initializeViews();
        setClickListeners();
    }

    private void initializeViews() {
        cardGames = findViewById(R.id.card_games);
        cardFirmware = findViewById(R.id.card_firmware);
        cardControls = findViewById(R.id.card_controls);
        cardSettings = findViewById(R.id.card_settings);
        cardUpdates = findViewById(R.id.card_updates);
        cardAbout = findViewById(R.id.card_about);
    }

    private void setClickListeners() {
        cardGames.setOnClickListener(this);
        cardFirmware.setOnClickListener(this);
        cardControls.setOnClickListener(this);
        cardSettings.setOnClickListener(this);
        cardUpdates.setOnClickListener(this);
        cardAbout.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        int id = v.getId();

        if (id == R.id.card_games) {
            openActivity(GamesActivity.class);
        } else if (id == R.id.card_firmware) {
            openActivity(FirmwareActivity.class);
        } else if (id == R.id.card_controls) {
            openActivity(ControlsActivity.class);
        } else if (id == R.id.card_settings) {
            openActivity(SettingsActivity.class);
        } else if (id == R.id.card_updates) {
            openActivity(UpdatesActivity.class);
        } else if (id == R.id.card_about) {
            openActivity(AboutActivity.class);
        } else {
            Toast.makeText(this, "Unknown option", Toast.LENGTH_SHORT).show();
        }
    }

    private void openActivity(Class<?> targetActivity) {
        try {
            android.content.Intent intent = new android.content.Intent(this, targetActivity);
            startActivity(intent);
        } catch (Exception e) {
            Toast.makeText(this, "Unable to open activity", Toast.LENGTH_SHORT).show();
            e.printStackTrace();
        }
    }
}