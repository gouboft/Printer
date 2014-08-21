package com.cmcc.printer;


import android.app.Activity;
import android.util.Log;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.view.View.OnClickListener;
import android.widget.EditText;


public class Printer extends Activity {
    private static String TAG = "Printer_JAVA";
    private Button initButton,testButton, printButton, clearButton;
    private EditText text;
    private byte[] version;

    public Printer() {
    }

    static {
        System.loadLibrary("CMCC_PRINT_BOSSTUN");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_printer);

        initButton = (Button)findViewById(R.id.button);
        testButton = (Button)findViewById(R.id.button1);
        printButton = (Button)findViewById(R.id.button2);
        clearButton = (Button)findViewById(R.id.button3);
        text = (EditText) findViewById(R.id.editText);

        initButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                openPrinter(4, null, null);
                version = new byte[3];
                getPrinterVersion(version);
                Log.d(TAG, "JNI Version: " + version[0] + version[1] + version[2]);
            }
        });

        testButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                byte bytes[] = {0x1B,0x40,0x12,0x54};
                String string = new String(bytes);
                Log.d(TAG, "Print the Test Page, Command: " + string);
                print(string);
            }
        });

        clearButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                text.setText("");
            }
        });

        printButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                String string = text.getText().toString();
                print(string);
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.printer, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private native int openPrinter(int printerType, String deviceId, String password);
    private native int closePrinter();
    private native int getPrinterVersion(byte[] version);
    private native int initialPrinter();
    private native int setZoonIn(int widthZoonIn,int heightZoonIn);
    private native int setAlignType(int alignType);
    private native int setLeftMargin(int n);
    private native int setRightMargin(int n);
    private native int setLineSpacingByDotPitch (int n);
    private native int setWordSpacingByDotPitch(int n);
    private native int setPrintOrientation (int printOrientation);
    private native int setBold(int n);
    private native int setUnderLine(int n);
    private native int setInverse(int n);
    private native int print(String content);
    private native int printHTML(String content);

}

