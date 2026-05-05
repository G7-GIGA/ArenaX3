package com.arenax3;

import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class AboutActivity extends AppCompatActivity {

    private TextView textViewAppName;
    private TextView textViewVersion;
    private TextView textViewBuildDate;
    private TextView textViewDescription;
    private TextView textViewCopyright;
    private Button buttonLicense;
    private Button buttonPrivacyPolicy;
    private Button buttonContact;
    private Button buttonRateApp;
    private Button buttonShareApp;
    private LinearLayout layoutCredits;
    private CardView cardOpenSource;
    private ImageView imageViewLogo;

    private static final String GITHUB_URL = "https://github.com/arenax3/emulator";
    private static final String WEBSITE_URL = "https://www.arenax3.com";
    private static final String CONTACT_EMAIL = "support@arenax3.com";
    private static final String PRIVACY_POLICY_URL = "https://www.arenax3.com/privacy";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);

        initializeViews();
        displayAppInfo();
        setupClickListeners();
        populateCredits();
        setupOpenSourceLicenses();
    }

    private void initializeViews() {
        textViewAppName = findViewById(R.id.text_view_app_name);
        textViewVersion = findViewById(R.id.text_view_version);
        textViewBuildDate = findViewById(R.id.text_view_build_date);
        textViewDescription = findViewById(R.id.text_view_description);
        textViewCopyright = findViewById(R.id.text_view_copyright);
        buttonLicense = findViewById(R.id.button_license);
        buttonPrivacyPolicy = findViewById(R.id.button_privacy_policy);
        buttonContact = findViewById(R.id.button_contact);
        buttonRateApp = findViewById(R.id.button_rate_app);
        buttonShareApp = findViewById(R.id.button_share_app);
        layoutCredits = findViewById(R.id.layout_credits);
        cardOpenSource = findViewById(R.id.card_open_source);
        imageViewLogo = findViewById(R.id.image_view_logo);

        // Make links clickable
        textViewDescription.setMovementMethod(LinkMovementMethod.getInstance());
    }

    private void displayAppInfo() {
        try {
            PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            String versionName = packageInfo.versionName;
            int versionCode = packageInfo.versionCode;
            
            textViewAppName.setText("ArenaX3");
            textViewVersion.setText("Version " + versionName + " (" + versionCode + ")");
            
            // Build date - in production, get from BuildConfig or manifest meta-data
            SimpleDateFormat dateFormat = new SimpleDateFormat("MMMM dd, yyyy", Locale.getDefault());
            textViewBuildDate.setText("Build Date: " + dateFormat.format(new Date()));
            
            textViewDescription.setText(
                "ArenaX3 is a high-performance PlayStation 3 emulator for Android.\n\n" +
                "Experience your favorite PS3 games on mobile devices with enhanced graphics, " +
                "smooth performance, and customizable controls.\n\n" +
                "Visit: " + WEBSITE_URL + "\n" +
                "GitHub: " + GITHUB_URL
            );
            
            textViewCopyright.setText("© 2024-2025 ArenaX3 Team. All rights reserved.\n" +
                "This project is not affiliated with or endorsed by Sony Interactive Entertainment.");
            
        } catch (PackageManager.NameNotFoundException e) {
            textViewVersion.setText("Version: Unknown");
            e.printStackTrace();
        }
    }

    private void setupClickListeners() {
        buttonLicense.setOnClickListener(v -> showLicenseDialog());
        
        buttonPrivacyPolicy.setOnClickListener(v -> {
            Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(PRIVACY_POLICY_URL));
            startActivity(browserIntent);
        });
        
        buttonContact.setOnClickListener(v -> {
            Intent emailIntent = new Intent(Intent.ACTION_SENDTO);
            emailIntent.setData(Uri.parse("mailto:" + CONTACT_EMAIL));
            emailIntent.putExtra(Intent.EXTRA_SUBJECT, "ArenaX3 Support Inquiry");
            emailIntent.putExtra(Intent.EXTRA_TEXT, "\n\n---\nApp Version: " + getAppVersion() + "\nDevice: " + Build.MANUFACTURER + " " + Build.MODEL + "\nAndroid: " + Build.VERSION.RELEASE);
            
            try {
                startActivity(Intent.createChooser(emailIntent, "Contact Support"));
            } catch (Exception e) {
                Toast.makeText(this, "No email app found", Toast.LENGTH_SHORT).show();
            }
        });
        
        buttonRateApp.setOnClickListener(v -> rateApp());
        
        buttonShareApp.setOnClickListener(v -> shareApp());
        
        cardOpenSource.setOnClickListener(v -> {
            Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(GITHUB_URL));
            startActivity(browserIntent);
        });
    }

    private void populateCredits() {
        String[][] creditsList = {
            {"Lead Developer", "Ahmed Hassan"},
            {"Emulation Core", "RPCS3 Team"},
            {"Graphics Engine", "Vulkan/OpenGL Contributors"},
            {"Audio System", "OpenAL Team"},
            {"UI/UX Design", "ArenaX3 Design Team"},
            {"Testing & QA", "Community Contributors"},
            {"Special Thanks", "All Open Source Contributors"}
        };
        
        for (String[] credit : creditsList) {
            addCreditItem(credit[0], credit[1]);
        }
    }

    private void addCreditItem(String role, String name) {
        View creditView = getLayoutInflater().inflate(R.layout.item_credit, null);
        TextView textRole = creditView.findViewById(R.id.text_role);
        TextView textName = creditView.findViewById(R.id.text_name);
        
        textRole.setText(role);
        textName.setText(name);
        
        layoutCredits.addView(creditView);
    }

    private void setupOpenSourceLicenses() {
        // Card is already set up, we can add click listener to show licenses
        cardOpenSource.setOnClickListener(v -> showOpenSourceLicensesDialog());
    }

    private void showLicenseDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("License");
        builder.setMessage(
            "ArenaX3 Emulator\n\n" +
            "Licensed under the GNU General Public License v3.0\n\n" +
            "This program is free software: you can redistribute it and/or modify\n" +
            "it under the terms of the GNU General Public License as published by\n" +
            "the Free Software Foundation, either version 3 of the License, or\n" +
            "(at your option) any later version.\n\n" +
            "This program is distributed in the hope that it will be useful,\n" +
            "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" +
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n" +
            "GNU General Public License for more details.\n\n" +
            "You should have received a copy of the GNU General Public License\n" +
            "along with this program. If not, see <https://www.gnu.org/licenses/>.\n\n" +
            "====================================\n\n" +
            "Third-Party Libraries:\n\n" +
            "• RPCS3 - GPLv2 License\n" +
            "• Vulkan - Khronos License\n" +
            "• OpenGL ES - Khronos License\n" +
            "• Volley - Apache 2.0 License\n" +
            "• Material Components - Apache 2.0 License"
        );
        builder.setPositiveButton("Close", null);
        builder.setNeutralButton("View Full GPLv3", (dialog, which) -> {
            Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://www.gnu.org/licenses/gpl-3.0.txt"));
            startActivity(browserIntent);
        });
        builder.show();
    }

    private void showOpenSourceLicensesDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Open Source Licenses");
        builder.setMessage(
            "ArenaX3 uses the following open source components:\n\n" +
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" +
            "RPCS3 PS3 Emulator Core\n" +
            "License: GPLv2\n" +
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
            
            "Vulkan Graphics API\n" +
            "License: Khronos License\n" +
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
            
            "OpenGL ES\n" +
            "License: Khronos License\n" +
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
            
            "Google Volley\n" +
            "License: Apache 2.0\n" +
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
            
            "Material Components for Android\n" +
            "License: Apache 2.0\n" +
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
            
            "AndroidX Libraries\n" +
            "License: Apache 2.0\n" +
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
            
            "For complete license texts, please visit our GitHub repository."
        );
        builder.setPositiveButton("Close", null);
        builder.setNeutralButton("View on GitHub", (dialog, which) -> {
            Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(GITHUB_URL));
            startActivity(browserIntent);
        });
        builder.show();
    }

    private void rateApp() {
        try {
            Intent rateIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=" + getPackageName()));
            startActivity(rateIntent);
        } catch (Exception e) {
            // Fallback to browser
            Intent browserIntent = new Intent(Intent.ACTION_VIEW, 
                Uri.parse("https://play.google.com/store/apps/details?id=" + getPackageName()));
            startActivity(browserIntent);
        }
    }

    private void shareApp() {
        Intent shareIntent = new Intent(Intent.ACTION_SEND);
        shareIntent.setType("text/plain");
        shareIntent.putExtra(Intent.EXTRA_SUBJECT, "ArenaX3 Emulator");
        shareIntent.putExtra(Intent.EXTRA_TEXT, 
            "Check out ArenaX3 - A high-performance PS3 emulator for Android!\n\n" +
            "Download it here: https://play.google.com/store/apps/details?id=" + getPackageName() + "\n\n" +
            "Experience PS3 games on your mobile device!");
        
        startActivity(Intent.createChooser(shareIntent, "Share ArenaX3"));
    }

    private String getAppVersion() {
        try {
            PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            return packageInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            return "Unknown";
        }
    }
}