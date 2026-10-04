package com.universalspoof.ui;

import android.app.Activity;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.util.Base64;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;

public final class MainActivity extends Activity {
    private WebView webView;
    private static final String CONFIG = "/data/adb/simspoof.prop";
    private static final String[] FIELDS = {
        "model", "brand", "device", "product", "manufacturer", "name", "board", "hardware",
        "bootloader", "locale", "country", "country_iso", "serial", "fingerprint", "build_id",
        "build_display", "security_patch", "baseband", "soc_model", "soc_manufacturer", "board_platform",
        "host", "user", "signature", "network_operator", "network_operator_name", "sim_operator",
        "sim_operator_name", "timezone"
    };

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(Color.rgb(12, 17, 29));
        getWindow().setNavigationBarColor(Color.rgb(12, 17, 29));
        webView = new WebView(this);
        webView.setBackgroundColor(Color.rgb(12, 17, 29));
        webView.getSettings().setJavaScriptEnabled(true);
        webView.getSettings().setDomStorageEnabled(false);
        webView.setWebChromeClient(new WebChromeClient());
        webView.setWebViewClient(new WebViewClient());
        webView.addJavascriptInterface(new Bridge(), "Native");
        setContentView(webView);
        webView.loadUrl("file:///android_asset/index.html");
    }

    @Override public void onBackPressed() {
        if (webView != null && webView.canGoBack()) webView.goBack(); else super.onBackPressed();
    }

    public final class Bridge {
        @JavascriptInterface public String getApps() {
            JSONArray out = new JSONArray();
            PackageManager pm = getPackageManager();
            try {
                for (PackageInfo p : pm.getInstalledPackages(0)) {
                    if (p.applicationInfo == null) continue;
                    JSONObject item = new JSONObject();
                    item.put("package", p.packageName);
                    CharSequence label = p.applicationInfo.loadLabel(pm);
                    item.put("label", label == null ? p.packageName : label.toString());
                    item.put("system", (p.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0);
                    out.put(item);
                }
            } catch (Exception ignored) {}
            return out.toString();
        }

        @JavascriptInterface public String getProfile(String pkg) {
            if (!validPackage(pkg)) return "{}";
            try {
                String data = rootRead();
                String prefix = "app." + pkg + ".";
                JSONObject out = new JSONObject();
                for (String line : data.split("\\r?\\n")) {
                    int eq = line.indexOf('=');
                    if (eq <= 0 || !line.startsWith(prefix)) continue;
                    String key = line.substring(prefix.length(), eq);
                    if (!key.startsWith("_pref_")) out.put(key, line.substring(eq + 1));
                }
                return out.toString();
            } catch (Exception e) {
                return new JSONObject().toString();
            }
        }

        @JavascriptInterface public String saveProfile(String pkg, String mode, boolean active, boolean allowed, String fieldsJson) {
            if (!validPackage(pkg)) return result(false, "Invalid package name");
            if (!"native".equals(mode) && !"lsposed".equals(mode)) return result(false, "Invalid hook mode");
            try {
                JSONObject input = new JSONObject(fieldsJson == null ? "{}" : fieldsJson);
                TreeMap<String,String> values = new TreeMap<>();
                String existing = rootRead();
                String prefix = "app." + pkg + ".";
                // Preserve unknown fields and SimSpoofer's serialized preferences for this app.
                for (String line : existing.split("\\r?\\n")) {
                    int eq = line.indexOf('=');
                    if (eq <= 0 || !line.startsWith(prefix)) continue;
                    values.put(line.substring(prefix.length(), eq), line.substring(eq + 1));
                }
                values.put("hook_mode", mode);
                values.put("active", active ? "true" : "false");
                values.put("allowed", allowed ? "true" : "false");
                for (String key : FIELDS) {
                    String value = input.optString(key, "").replace('\r',' ').replace('\n',' ').trim();
                    if (value.isEmpty()) values.remove(key); else values.put(key, value);
                }
                StringBuilder next = new StringBuilder("format=simspoof-v1\nmanaged=1\nscope=per_app\n");
                for (String line : existing.split("\\r?\\n")) {
                    if (line.trim().isEmpty() || line.startsWith("format=") || line.startsWith("managed=") ||
                        line.startsWith("scope=") || line.startsWith("updated_at=") || line.startsWith(prefix)) continue;
                    next.append(line).append('\n');
                }
                for (Map.Entry<String,String> entry : values.entrySet())
                    next.append(prefix).append(entry.getKey()).append('=').append(entry.getValue()).append('\n');
                next.append("updated_at=").append(System.currentTimeMillis()).append('\n');
                rootWrite(next.toString());
                return result(true, "Saved profile for " + pkg + ". Force-stop and reopen the target app to reload it.");
            } catch (Exception e) {
                return result(false, "Save failed: " + e.getMessage());
            }
        }

        @JavascriptInterface public String getConfigStatus() {
            try {
                String config = rootRead();
                JSONObject out = new JSONObject();
                out.put("readable", true);
                out.put("path", CONFIG);
                out.put("appProfiles", countProfiles(config));
                out.put("scope", config.contains("scope=per_app") ? "per_app" : "unknown/global legacy");
                return out.toString();
            } catch (Exception e) {
                JSONObject out = new JSONObject();
                try { out.put("readable", false); out.put("error", e.getMessage()); } catch (JSONException ignored) {}
                return out.toString();
            }
        }
    }

    private static boolean validPackage(String pkg) {
        return pkg != null && pkg.length() <= 255 && pkg.matches("[A-Za-z0-9_]+(\\.[A-Za-z0-9_]+)+");
    }
    private static String result(boolean ok, String message) {
        JSONObject out = new JSONObject();
        try { out.put("ok", ok); out.put("message", message); } catch (JSONException ignored) {}
        return out.toString();
    }
    private static int countProfiles(String config) {
        java.util.HashSet<String> packages = new java.util.HashSet<>();
        for (String line : config.split("\\r?\\n")) {
            if (!line.startsWith("app.")) continue;
            for (String suffix : new String[]{".hook_mode=", ".active=", ".allowed="}) {
                int at = line.indexOf(suffix);
                if (at > 4) { packages.add(line.substring(4, at)); break; }
            }
        }
        return packages.size();
    }
    private static String rootRead() throws Exception {
        Process p = new ProcessBuilder("su", "-c", "if [ -r " + CONFIG + " ]; then cat " + CONFIG + "; else exit 3; fi").redirectErrorStream(true).start();
        StringBuilder b = new StringBuilder();
        try (BufferedReader r = new BufferedReader(new InputStreamReader(p.getInputStream(), StandardCharsets.UTF_8))) {
            String line; while ((line = r.readLine()) != null) b.append(line).append('\n');
        }
        int exit = p.waitFor();
        if (exit != 0) throw new IllegalStateException("su read failed: " + exit);
        return b.toString();
    }
    private static void rootWrite(String config) throws Exception {
        String encoded = Base64.encodeToString(config.getBytes(StandardCharsets.UTF_8), Base64.NO_WRAP);
        String command = "mkdir -p /data/adb && echo '" + encoded + "' | base64 -d > " + CONFIG + ".tmp && " +
                "chmod 0644 " + CONFIG + ".tmp && mv " + CONFIG + ".tmp " + CONFIG;
        Process p = new ProcessBuilder("su", "-c", command).redirectErrorStream(true).start();
        StringBuilder output = new StringBuilder();
        try (BufferedReader r = new BufferedReader(new InputStreamReader(p.getInputStream(), StandardCharsets.UTF_8))) {
            String line; while ((line = r.readLine()) != null) output.append(line).append('\n');
        }
        int exit = p.waitFor();
        if (exit != 0) throw new IllegalStateException("su write failed: " + exit + " " + output);
    }
}
