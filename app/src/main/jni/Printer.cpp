
/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <termios.h>
#include <jni.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


#include <android/log.h>
#define LOG_TAG "Printer_JNI"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


extern "C"
{
    jint JNI_OnLoad(JavaVM *jvm, void *reserved);
}


namespace android
{
const char *gNativePrinterClassName = "com/cmcc/printer/Printer";
static int fd;
static char PrinterDev[] = "/dev/ttyS0";
static uint8_t Version[] = {0,0,1};

# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

#define SUCCESS                 0
#define FAIL                    2000
#define FIND_PRINTER_FAIL       2001
#define CONNECT_PRINTER_FAIL    2002
#define BLUETOOTH_PASSWD_ERR    2003
#define PARAM_RANGE_ERR         2004
#define PARAM_FORMAT_ERR        2005
#define LACKING_PAPER           2006

static speed_t getBaudrate(int baudrate)
{
    switch(baudrate) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 500000: return B500000;
    case 576000: return B576000;
    default: return 1;
    }
}

static int initSerialPort(char *path, int baudrate, int flags)
{
    speed_t speed;
    int error = -1;

    /* Check arguments */
    {
        speed = getBaudrate(baudrate);
        if (speed == 1) {
            ALOGE("Invalid baudrate");
            return error;
        }
    }

    /* Opening device */
    {
        jboolean iscopy;
        ALOGD("Opening serial port %s with flags 0x%x", path, O_RDWR | flags);
        if (fd > 0)
            close(fd);
        fd = open(path, O_RDWR | flags);
        ALOGD("open() fd = %d", fd);
        if (fd == -1)
        {
            ALOGE("Cannot open port");
            return error;
        }
    }

    /* Configure device */
    {
        struct termios cfg;
        ALOGD("Configuring serial port");
        if (tcgetattr(fd, &cfg))
        {
            ALOGE("tcgetattr() failed");
            close(fd);
            return error;
        }

        cfmakeraw(&cfg);
        cfsetispeed(&cfg, speed);
        cfsetospeed(&cfg, speed);

        if (tcsetattr(fd, TCSANOW, &cfg))
        {
            ALOGE("tcsetattr() failed");
            close(fd);
            return error;
        }
    }
    
    return 0;
}
   

static int Jni_openPrinter(JNIEnv* e, jobject o, jint printerType, jstring deviceId, jstring password)
{
    int ret;
    if (printerType != 4){
        ALOGE("printerType error, this driver is for internel printer!");
        return FIND_PRINTER_FAIL;
    }
    

    ret = initSerialPort(PrinterDev, 115200, 0);
    if (ret < 0) {
        ALOGE("Initialize serial port fail!");
        return CONNECT_PRINTER_FAIL;
    }
    
    return SUCCESS;

}

static int Jni_closePrinter(JNIEnv* e, jobject o)
{
    close(fd);
    return SUCCESS;
}

static int Jni_getPrinterVersion(JNIEnv* e, jobject o, jbyteArray version)
{
    e->SetByteArrayRegion(version, 0, 3, (jbyte*) Version);

    jbyte *bytes = e->GetByteArrayElements(version, 0);
    char *ver = (char *) bytes;

    e->ReleaseByteArrayElements(version, bytes, 0);
    ALOGD("JNI Version: %d%d%d", ver[0], ver[1], ver[2]);
    return SUCCESS;
}

static int Jni_initialPrinter(JNIEnv* e, jobject o)
{
    char buf[21];
    
    //reset printer
    buf[0] = 0x1B;
    buf[1] = 0x40;
    
    //set zoon in to 1
    buf[2] = 0x1D;
    buf[3] = 0x21;
    buf[4] = 0x00;

    //set align left
    buf[5] = 0x1B;
    buf[6] = 0x61;
    buf[7] = 0x48;

    //set left margin to 0
    buf[8] = 0x1B;
    buf[9] = 0x4C;
    buf[10] = 0x00;
    buf[11] = 0x00;

    //set right margin to 0
    //No need to set
    
    //set line spacing
    buf[12] = 0x1B;
    buf[13] = 0x33;
    buf[14] = 0x08;

    //set character spacing
    //No need to set
    
    //set print direction: Horizontal
    //No need to set
    
    //set bold: Regular
    //No need to set
    
    //set underline: Regular
    buf[15] = 0x1B;
    buf[16] = 0x2D;
    buf[17] = 0x48;
    
    //set inverse: Regular
    buf[18] = 0x1D;
    buf[19] = 0x42;
    buf[20] = 0x00;

    if(fd > 0)
        write(fd, buf, sizeof(buf));
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }
 
    return SUCCESS;
}

static int Jni_setZoonIn(JNIEnv* e, jobject o, jint widthZoonIn,jint heightZoonIn)
{
    char buf[3];
    int width, height;
        
    buf[0] = 0x1D;
    buf[1] = 0x21;

    if (widthZoonIn >= 1 && widthZoonIn <= 8)
        width = (widthZoonIn - 1) << 4;
    else
        return PARAM_RANGE_ERR;
    if (heightZoonIn >= 1 && heightZoonIn <= 8)
        height = widthZoonIn - 1;
    else
        return PARAM_RANGE_ERR;

    buf[2] = (char) (width | height);

    if(fd > 0)
        write(fd, buf, sizeof(buf));
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }
 
    return SUCCESS;
}

static int Jni_setAlignType(JNIEnv* e, jobject o, jint alignType)
{
    char buf[3];

    if (alignType < 0 || alignType > 2)
        return PARAM_RANGE_ERR;

    buf[0] = 0x1B;
    buf[1] = 0x61;

    buf[2] = (char) alignType;

    if(fd > 0)
        write(fd, buf, sizeof(buf));
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }

    return SUCCESS;
}

static int Jni_setLeftMargin(JNIEnv* e, jobject o, jint n)
{
    char buf[4];

    if (n < 0 || n > 0xFFFF)
        return PARAM_RANGE_ERR;

    buf[0] = 0x1D;
    buf[1] = 0x4C;

    if (n <= 256) {
        buf[2] = (char) n;
        buf[3] = 0x00;
    } else if (n >= 256) {
        buf[2] = (char)(0xFF & n);
        buf[3] = (char)(((0xFF << 8) & n) >> 8);
    }

    if(fd > 0)
        write(fd, buf, sizeof(buf));
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }

    return SUCCESS;
}

static int Jni_setRightMargin(JNIEnv* e, jobject o, jint n)
{
    //当前使用的打印机没有该设置
    return SUCCESS;
}

//设置打印机字符串的字符行间距为 n 个垂直点距
static int Jni_setLineSpacingByDotPitch (JNIEnv* e, jobject o, jint n)
{
    char buf[3];

    if (n < 0 || n > 0xFF)
        return PARAM_RANGE_ERR;

    buf[0] = 0x1B;
    buf[1] = 0x33;
    buf[2] = (char) n;

    if(fd > 0)
        write(fd, buf, sizeof(buf));
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }

    return SUCCESS;
}

static int Jni_setWordSpacingByDotPitch(JNIEnv* e, jobject o, jint n)
{
    //当前使用的打印机没有该设置
    if (n < 0 || n > 0xFF)
        return PARAM_RANGE_ERR;

    return SUCCESS;
}

static int Jni_setPrintOrientation (JNIEnv* e, jobject o, jint printOrientation)
{
    char buf[3];

    if (printOrientation != 0 || printOrientation != 1)
        return PARAM_RANGE_ERR;

    buf[0] = 0x1B;
    buf[1] = 0x56;
    buf[2] = (char) printOrientation;

    if(fd > 0)
        write(fd, buf, sizeof(buf));
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }

    return SUCCESS;
}

static int Jni_setBold(JNIEnv* e, jobject o, jint n)
{
    //当前使用的打印机没有该设置
    return SUCCESS;
}

static int Jni_setUnderLine(JNIEnv* e, jobject o, jint n)
{
    char buf[3];

    if (n != 0 || n != 1)
        return PARAM_RANGE_ERR;

    buf[0] = 0x1B;
    buf[1] = 0x2D;
    buf[2] = (char) n;

    if(fd > 0)
        write(fd, buf, sizeof(buf));
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }

    return SUCCESS;
}

static int Jni_setInverse(JNIEnv* e, jobject o, jint n)
{
    char buf[3];

    if (n != 0 || n != 1)
        return PARAM_RANGE_ERR;

    buf[0] = 0x1D;
    buf[1] = 0x42;
    buf[2] = (char) n;

    if(fd > 0)
        write(fd, buf, sizeof(buf));
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }

    return SUCCESS;
}

static int Jni_print(JNIEnv* e, jobject o, jstring content)
{
    char *printContent = (char *) e->GetStringUTFChars(content, NULL);
    int length = (int) e->GetStringUTFLength(content);

    ALOGD("String Content: %s", printContent);

    if(fd > 0)
        write(fd, printContent, length);
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }

    return SUCCESS;
}

static int Jni_printHTML(JNIEnv* e, jobject o, jstring content)
{
    char *printContent = (char *) e->GetStringUTFChars(content, NULL);
    int length = (int) e->GetStringUTFLength(content);
    
    if(fd > 0)
        write(fd, printContent, length);
    else {
        ALOGE("printer device not open!");
        return FIND_PRINTER_FAIL;
    }

    return SUCCESS;
}

/*****************************************************************************
 **
 ** JNI functions for android-4.0.1_r1
 **
 *****************************************************************************/
static JNINativeMethod gMethods[] =
{
    {"openPrinter", "(ILjava/lang/String;Ljava/lang/String;)I",
     (void *)Jni_openPrinter},
    {"closePrinter", "()I",
     (void *)Jni_closePrinter},
    {"getPrinterVersion", "([B)I",
     (void *)Jni_getPrinterVersion},
    {"initialPrinter", "()I",
     (void *)Jni_initialPrinter},
    {"setZoonIn", "(II)I",
     (void *)Jni_setZoonIn},
    {"setAlignType", "(I)I",
     (void *)Jni_setAlignType},
    {"setLeftMargin", "(I)I",
     (void *)Jni_setLeftMargin},
    {"setRightMargin", "(I)I",
     (void *)Jni_setLeftMargin},
    {"setLineSpacingByDotPitch", "(I)I",
     (void *)Jni_setLineSpacingByDotPitch},
    {"setWordSpacingByDotPitch", "(I)I",
     (void *)Jni_setWordSpacingByDotPitch},
    {"setPrintOrientation", "(I)I",
     (void *)Jni_setPrintOrientation},
    {"setBold", "(I)I",
     (void *)Jni_setBold},
    {"setUnderLine", "(I)I",
     (void *)Jni_setUnderLine},
    {"setInverse", "(I)I",
     (void *)Jni_setInverse},
    {"print", "(Ljava/lang/String;)I",
     (void *)Jni_print},
    {"printHTML", "(Ljava/lang/String;)I",
     (void *)Jni_printHTML},

};

static int registerNativeMethods(JNIEnv* env, const char* className,
        JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}



static int register_Printer (JNIEnv *e)
{
    ALOGD ("In %s", __FUNCTION__);
    return registerNativeMethods(e, gNativePrinterClassName, gMethods, NELEM(gMethods));
}

}

/*******************************************************************************
 **
 ** Function:        JNI_OnLoad
 **
 ** Description:     Register all JNI functions with Java Virtual Machine.
 **                  jvm: Java Virtual Machine.
 **                  reserved: Not used.
 **
 ** Returns:         JNI version.
 **
 *******************************************************************************/
jint JNI_OnLoad (JavaVM* jvm, void*)
{
    ALOGD("Printer: loading JNI");

    JNIEnv *e = NULL;

    // Check JNI version
    if (jvm->GetEnv ((void **) &e, JNI_VERSION_1_6))
        return JNI_ERR;

    if (android::register_Printer (e) == -1)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}

/*
native int openPrinter(int printerType, String deviceId, String password)
native int closePrinter()
native int getPrinterVersion(byte[] version)
native int initialPrinter()
native int setZoonIn(int widthZoonIn,int heightZoonIn)
native int setAlignType(int alignType) 
native int setLeftMargin(int n) 
native int setRightMargin(int n) 
native int setLineSpacingByDotPitch (int n)
native int setWordSpacingByDotPitch(int n) 
native int setPrintOrientation (int printOrientation)
native int setBold(int n)
native int setUnderLine(int n) 
native int setInverse(int n) 
native int print(String content) 
native int printHTML(String content)
*/
