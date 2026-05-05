package com.arenax3;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.Gravity;
import android.widget.ImageView;

public class ControlsActivity extends AppCompatActivity {

    private SharedPreferences sharedPreferences;
    private static final String PREFS_NAME = "ArenaX3Controls";
    
    // Touch controls
    private Switch switchTouchControls;
    private Switch switchVibration;
    private Switch switchPressureSensitivity;
    private SeekBar seekBarTouchSize;
    private SeekBar seekBarTouchOpacity;
    private TextView textTouchSizeValue;
    private TextView textTouchOpacityValue;
    private RadioGroup radioGroupLayout;
    private Button buttonCustomizeLayout;
    private Button buttonResetTouch;
    
    // Gamepad mapping
    private Switch switchGamepadSupport;
    private Button buttonMapKeys;
    private Button buttonResetMapping;
    private LinearLayout layoutGamepadInfo;
    private TextView textGamepadStatus;
    private RadioGroup radioGroupGamepadType;
    
    // Preview
    private FrameLayout framePreview;
    private TouchPreviewView previewView;
    private boolean isCustomizing = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_controls);
        
        sharedPreferences = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        
        initializeViews();
        loadSettings();
        setupClickListeners();
        setupPreview();
    }
    
    private void initializeViews() {
        // Touch controls
        switchTouchControls = findViewById(R.id.switch_touch_controls);
        switchVibration = findViewById(R.id.switch_vibration);
        switchPressureSensitivity = findViewById(R.id.switch_pressure_sensitivity);
        seekBarTouchSize = findViewById(R.id.seek_bar_touch_size);
        seekBarTouchOpacity = findViewById(R.id.seek_bar_touch_opacity);
        textTouchSizeValue = findViewById(R.id.text_touch_size_value);
        textTouchOpacityValue = findViewById(R.id.text_touch_opacity_value);
        radioGroupLayout = findViewById(R.id.radio_group_layout);
        buttonCustomizeLayout = findViewById(R.id.button_customize_layout);
        buttonResetTouch = findViewById(R.id.button_reset_touch);
        
        // Gamepad mapping
        switchGamepadSupport = findViewById(R.id.switch_gamepad_support);
        buttonMapKeys = findViewById(R.id.button_map_keys);
        buttonResetMapping = findViewById(R.id.button_reset_mapping);
        layoutGamepadInfo = findViewById(R.id.layout_gamepad_info);
        textGamepadStatus = findViewById(R.id.text_gamepad_status);
        radioGroupGamepadType = findViewById(R.id.radio_group_gamepad_type);
        
        // Preview
        framePreview = findViewById(R.id.frame_preview);
    }
    
    private void setupPreview() {
        previewView = new TouchPreviewView(this);
        framePreview.addView(previewView, new FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT
        ));
    }
    
    private void loadSettings() {
        // Load touch settings
        switchTouchControls.setChecked(sharedPreferences.getBoolean("touch_enabled", true));
        switchVibration.setChecked(sharedPreferences.getBoolean("vibration_enabled", true));
        switchPressureSensitivity.setChecked(sharedPreferences.getBoolean("pressure_sensitivity", false));
        
        int touchSize = sharedPreferences.getInt("touch_size", 70);
        int touchOpacity = sharedPreferences.getInt("touch_opacity", 80);
        
        seekBarTouchSize.setProgress(touchSize);
        seekBarTouchOpacity.setProgress(touchOpacity);
        textTouchSizeValue.setText(touchSize + "%");
        textTouchOpacityValue.setText(touchOpacity + "%");
        
        int layoutType = sharedPreferences.getInt("control_layout", 0);
        switch (layoutType) {
            case 0:
                radioGroupLayout.check(R.id.radio_layout_classic);
                break;
            case 1:
                radioGroupLayout.check(R.id.radio_layout_modern);
                break;
            case 2:
                radioGroupLayout.check(R.id.radio_layout_minimal);
                break;
        }
        
        // Load gamepad settings
        switchGamepadSupport.setChecked(sharedPreferences.getBoolean("gamepad_enabled", false));
        layoutGamepadInfo.setVisibility(switchGamepadSupport.isChecked() ? View.VISIBLE : View.GONE);
        
        int gamepadType = sharedPreferences.getInt("gamepad_type", 0);
        switch (gamepadType) {
            case 0:
                radioGroupGamepadType.check(R.id.radio_gamepad_xbox);
                break;
            case 1:
                radioGroupGamepadType.check(R.id.radio_gamepad_playstation);
                break;
            case 2:
                radioGroupGamepadType.check(R.id.radio_gamepad_generic);
                break;
        }
        
        boolean isMapped = sharedPreferences.getBoolean("keys_mapped", false);
        if (isMapped) {
            textGamepadStatus.setText("✓ Gamepad mapped and ready");
            textGamepadStatus.setTextColor(ContextCompat.getColor(this, android.R.color.holo_green_light));
        } else {
            textGamepadStatus.setText("⚠ No mapping configured");
            textGamepadStatus.setTextColor(ContextCompat.getColor(this, android.R.color.holo_orange_light));
        }
        
        updateTouchControlsVisibility();
    }
    
    private void setupClickListeners() {
        switchTouchControls.setOnCheckedChangeListener((buttonView, isChecked) -> {
            sharedPreferences.edit().putBoolean("touch_enabled", isChecked).apply();
            updateTouchControlsVisibility();
            if (isChecked && isCustomizing) {
                startCustomization();
            } else if (!isChecked && isCustomizing) {
                stopCustomization();
            }
            Toast.makeText(this, isChecked ? "Touch controls enabled" : "Touch controls disabled", Toast.LENGTH_SHORT).show();
        });
        
        switchVibration.setOnCheckedChangeListener((buttonView, isChecked) -> {
            sharedPreferences.edit().putBoolean("vibration_enabled", isChecked).apply();
        });
        
        switchPressureSensitivity.setOnCheckedChangeListener((buttonView, isChecked) -> {
            sharedPreferences.edit().putBoolean("pressure_sensitivity", isChecked).apply();
            Toast.makeText(this, isChecked ? "Pressure sensitivity enabled" : "Pressure sensitivity disabled", Toast.LENGTH_SHORT).show();
        });
        
        seekBarTouchSize.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    textTouchSizeValue.setText(progress + "%");
                    sharedPreferences.edit().putInt("touch_size", progress).apply();
                    previewView.setControlSize(progress);
                }
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        
        seekBarTouchOpacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    textTouchOpacityValue.setText(progress + "%");
                    sharedPreferences.edit().putInt("touch_opacity", progress).apply();
                    int alpha = (int) (255 * (progress / 100f));
                    previewView.setControlsAlpha(alpha);
                }
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        
        radioGroupLayout.setOnCheckedChangeListener((group, checkedId) -> {
            int layoutType;
            if (checkedId == R.id.radio_layout_classic) layoutType = 0;
            else if (checkedId == R.id.radio_layout_modern) layoutType = 1;
            else layoutType = 2;
            
            sharedPreferences.edit().putInt("control_layout", layoutType).apply();
            previewView.setLayoutStyle(layoutType);
            Toast.makeText(this, "Layout changed", Toast.LENGTH_SHORT).show();
        });
        
        buttonCustomizeLayout.setOnClickListener(v -> {
            if (switchTouchControls.isChecked()) {
                if (isCustomizing) {
                    stopCustomization();
                } else {
                    startCustomization();
                }
            } else {
                Toast.makeText(this, "Enable touch controls first", Toast.LENGTH_SHORT).show();
            }
        });
        
        buttonResetTouch.setOnClickListener(v -> {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("Reset Touch Controls");
            builder.setMessage("Reset all touch control settings to default?");
            builder.setPositiveButton("Reset", (dialog, which) -> {
                sharedPreferences.edit()
                    .putBoolean("touch_enabled", true)
                    .putBoolean("vibration_enabled", true)
                    .putBoolean("pressure_sensitivity", false)
                    .putInt("touch_size", 70)
                    .putInt("touch_opacity", 80)
                    .putInt("control_layout", 0)
                    .apply();
                loadSettings();
                Toast.makeText(this, "Touch controls reset to default", Toast.LENGTH_SHORT).show();
            });
            builder.setNegativeButton("Cancel", null);
            builder.show();
        });
        
        // Gamepad listeners
        switchGamepadSupport.setOnCheckedChangeListener((buttonView, isChecked) -> {
            sharedPreferences.edit().putBoolean("gamepad_enabled", isChecked).apply();
            layoutGamepadInfo.setVisibility(isChecked ? View.VISIBLE : View.GONE);
            Toast.makeText(this, isChecked ? "Gamepad support enabled" : "Gamepad support disabled", Toast.LENGTH_SHORT).show();
        });
        
        buttonMapKeys.setOnClickListener(v -> showKeyMappingDialog());
        
        buttonResetMapping.setOnClickListener(v -> {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("Reset Gamepad Mapping");
            builder.setMessage("Reset all gamepad mappings to default?");
            builder.setPositiveButton("Reset", (dialog, which) -> {
                sharedPreferences.edit()
                    .putBoolean("keys_mapped", false)
                    .remove("gamepad_map_a")
                    .remove("gamepad_map_b")
                    .remove("gamepad_map_x")
                    .remove("gamepad_map_y")
                    .remove("gamepad_map_start")
                    .remove("gamepad_map_select")
                    .apply();
                textGamepadStatus.setText("⚠ No mapping configured");
                textGamepadStatus.setTextColor(ContextCompat.getColor(this, android.R.color.holo_orange_light));
                Toast.makeText(this, "Gamepad mapping reset", Toast.LENGTH_SHORT).show();
            });
            builder.setNegativeButton("Cancel", null);
            builder.show();
        });
        
        radioGroupGamepadType.setOnCheckedChangeListener((group, checkedId) -> {
            int gamepadType;
            if (checkedId == R.id.radio_gamepad_xbox) gamepadType = 0;
            else if (checkedId == R.id.radio_gamepad_playstation) gamepadType = 1;
            else gamepadType = 2;
            
            sharedPreferences.edit().putInt("gamepad_type", gamepadType).apply();
            Toast.makeText(this, "Gamepad type changed", Toast.LENGTH_SHORT).show();
        });
    }
    
    private void updateTouchControlsVisibility() {
        boolean enabled = switchTouchControls.isChecked();
        seekBarTouchSize.setEnabled(enabled);
        seekBarTouchOpacity.setEnabled(enabled);
        radioGroupLayout.setEnabled(enabled);
        buttonCustomizeLayout.setEnabled(enabled);
        buttonResetTouch.setEnabled(enabled);
        switchVibration.setEnabled(enabled);
        switchPressureSensitivity.setEnabled(enabled);
        
        if (enabled) {
            framePreview.setVisibility(View.VISIBLE);
        } else {
            framePreview.setVisibility(View.GONE);
            if (isCustomizing) stopCustomization();
        }
    }
    
    private void startCustomization() {
        isCustomizing = true;
        buttonCustomizeLayout.setText("Save Layout");
        Toast.makeText(this, "Tap and drag controls to reposition", Toast.LENGTH_LONG).show();
        previewView.startCustomization();
    }
    
    private void stopCustomization() {
        isCustomizing = false;
        buttonCustomizeLayout.setText("Customize Layout");
        previewView.stopCustomization();
        saveCustomPositions();
    }
    
    private void saveCustomPositions() {
        // Save custom button positions from previewView
        float[] positions = previewView.getButtonPositions();
        if (positions != null) {
            SharedPreferences.Editor editor = sharedPreferences.edit();
            for (int i = 0; i < positions.length; i++) {
                editor.putFloat("custom_pos_" + i, positions[i]);
            }
            editor.apply();
            Toast.makeText(this, "Layout saved", Toast.LENGTH_SHORT).show();
        }
    }
    
    private void showKeyMappingDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Map Gamepad Keys");
        
        String[] actions = {"Button A", "Button B", "Button X", "Button Y", "Start", "Select"};
        builder.setItems(actions, (dialog, which) -> {
            String action = actions[which];
            Toast.makeText(this, "Press a button on your gamepad for: " + action, Toast.LENGTH_LONG).show();
            // In a real implementation, you would listen for gamepad button presses here
            simulateMapping(action);
        });
        
        builder.show();
    }
    
    private void simulateMapping(String action) {
        // Simulate mapping (in production, this would capture actual gamepad input)
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Map " + action);
        builder.setMessage("Press the desired button on your gamepad...\n\n(Tap 'Simulate' for demo)");
        builder.setPositiveButton("Simulate", (dialog, which) -> {
            String mappingKey = "gamepad_map_" + action.toLowerCase().replace(" ", "_");
            sharedPreferences.edit().putString(mappingKey, "BUTTON_" + System.currentTimeMillis()).apply();
            sharedPreferences.edit().putBoolean("keys_mapped", true).apply();
            textGamepadStatus.setText("✓ Gamepad mapped and ready");
            textGamepadStatus.setTextColor(ContextCompat.getColor(this, android.R.color.holo_green_light));
            Toast.makeText(this, action + " mapped successfully!", Toast.LENGTH_SHORT).show();
        });
        builder.setNegativeButton("Cancel", null);
        builder.show();
    }
    
    // Custom View for touch control preview
    private class TouchPreviewView extends View {
        private Paint buttonPaint;
        private RectF dpadRect;
        private RectF actionRect;
        private float buttonSize = 70f;
        private int alpha = 204;
        private int layoutStyle = 0;
        private boolean customizing = false;
        private float[] buttonPositions = new float[8];
        private int selectedButton = -1;
        private float lastTouchX, lastTouchY;
        
        public TouchPreviewView(android.content.Context context) {
            super(context);
            buttonPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            buttonPaint.setColor(0x88FFFFFF);
            dpadRect = new RectF();
            actionRect = new RectF();
            initPositions();
        }
        
        private void initPositions() {
            // Initialize default button positions
            for (int i = 0; i < buttonPositions.length; i++) {
                buttonPositions[i] = i * 50f;
            }
        }
        
        public void setControlSize(int percent) {
            buttonSize = 40 + (percent * 0.6f);
            invalidate();
        }
        
        public void setControlsAlpha(int alphaVal) {
            alpha = alphaVal;
            buttonPaint.setColor((alpha << 24) | 0xFFFFFF);
            invalidate();
        }
        
        public void setLayoutStyle(int style) {
            layoutStyle = style;
            invalidate();
        }
        
        public void startCustomization() {
            customizing = true;
        }
        
        public void stopCustomization() {
            customizing = false;
            selectedButton = -1;
        }
        
        public float[] getButtonPositions() {
            return buttonPositions;
        }
        
        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            
            int width = getWidth();
            int height = getHeight();
            
            buttonPaint.setAlpha(alpha);
            
            // D-Pad (left side)
            float dpadX = width * 0.15f;
            float dpadY = height - buttonSize * 1.5f;
            dpadRect.set(dpadX - buttonSize/2, dpadY - buttonSize/2, 
                        dpadX + buttonSize/2, dpadY + buttonSize/2);
            canvas.drawRoundRect(dpadRect, 15, 15, buttonPaint);
            canvas.drawText("D-Pad", dpadX - 25, dpadY + 5, buttonPaint);
            
            // Action buttons (right side)
            float actionX = width - width * 0.15f;
            float actionY = height - buttonSize * 1.5f;
            actionRect.set(actionX - buttonSize/2, actionY - buttonSize/2,
                          actionX + buttonSize/2, actionY + buttonSize/2);
            canvas.drawRoundRect(actionRect, 15, 15, buttonPaint);
            canvas.drawText("A/B/X/Y", actionX - 30, actionY + 5, buttonPaint);
            
            // Additional buttons based on layout style
            if (layoutStyle == 1) { // Modern layout
                float centerX = width / 2;
                float centerY = height - buttonSize;
                RectF centerRect = new RectF(centerX - buttonSize/2, centerY - buttonSize/2,
                                            centerX + buttonSize/2, centerY + buttonSize/2);
                canvas.drawRoundRect(centerRect, 15, 15, buttonPaint);
                canvas.drawText("Menu", centerX - 20, centerY + 5, buttonPaint);
            }
            
            if (customizing) {
                buttonPaint.setAlpha(100);
                buttonPaint.setColor(0xFF00FF00);
                canvas.drawCircle(dpadX, dpadY, buttonSize/1.5f, buttonPaint);
                canvas.drawCircle(actionX, actionY, buttonSize/1.5f, buttonPaint);
            }
        }
        
        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (!customizing) return super.onTouchEvent(event);
            
            float x = event.getX();
            float y = event.getY();
            
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    selectedButton = checkButtonHit(x, y);
                    if (selectedButton != -1) {
                        lastTouchX = x;
                        lastTouchY = y;
                    }
                    return true;
                case MotionEvent.ACTION_MOVE:
                    if (selectedButton != -1) {
                        float dx = x - lastTouchX;
                        float dy = y - lastTouchY;
                        updateButtonPosition(selectedButton, dx, dy);
                        lastTouchX = x;
                        lastTouchY = y;
                        invalidate();
                    }
                    return true;
                case MotionEvent.ACTION_UP:
                    selectedButton = -1;
                    return true;
            }
            return super.onTouchEvent(event);
        }
        
        private int checkButtonHit(float x, float y) {
            // Simple hit detection
            return (x > 100 && x < 300 && y > getHeight() - 200) ? 0 : 
                   (x > getWidth() - 300 && x < getWidth() - 100 && y > getHeight() - 200) ? 1 : -1;
        }
        
        private void updateButtonPosition(int button, float dx, float dy) {
            // Update positions
        }
    }
}