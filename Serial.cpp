//
//  Serial.cpp
//  
//
//  Created by NAKAJIMA Musashi on 12/10/07.
//  Copyright 2012年 __MyCompanyName__. All rights reserved.
//

#include "Serial.h"

Serial::Serial() {
	active = 0;
	done = 0;
    cb = NULL;
    //strcpy(device, MODEMDEVICE);
    if(prepareSerial())
        threadStart();
}

void Serial::setDevice(const char *d)
{
    threadStop();
    strcpy(device, d);
    prepareSerial();
    threadStart();
}

bool Serial::prepareSerial()
{
    vector <string> prefixMatch;
    
    prefixMatch.push_back("cu.");
    prefixMatch.push_back("tty.");
    
    DIR *dir;
    struct dirent *entry;
    dir = opendir("/dev");
    
    string deviceName	= "";
    int deviceCount		= 0;
    
    if (dir == NULL){
        printf("buildDeviceList(): error listing devices in /dev");
    } else {
        //for each device
        while((entry = readdir(dir)) != NULL){
            deviceName = (char *)entry->d_name;
            
            //we go through the prefixes
            for(int k = 0; k < (int)prefixMatch.size(); k++){
                //if the device name is longer than the prefix
                if( deviceName.size() > prefixMatch[k].size() ){
                    //do they match ?
                    if( deviceName.substr(0, prefixMatch[k].size()) == prefixMatch[k].c_str() ){
                        string dev = "/dev/";
                        deviceName = dev.append(deviceName);
                        deviceList.push_back(deviceName);
                        cout << "deviceList[" << deviceCount << "]: " << deviceName << endl;
                        deviceCount++;
                        break;
                    }
                }
            }
        }
        closedir(dir);
    }
    //strcpy(device, deviceList[6].c_str());
    fd = open(device, O_RDWR | O_NOCTTY );
    if (fd <0) {printf("error:serial device\n"); return false;}

    tcgetattr(fd,&oldtio); /* 現在のシリアルポートの設定を待避させる*/
    bzero(&newtio, sizeof(newtio)); /* 新しいポートの設定の構造体をクリアする */
    
    /* 
     BAUDRATE: ボーレートの設定．cfsetispeed と cfsetospeed も使用できる．
     CRTSCTS : 出力のハードウェアフロー制御 (必要な結線が全てされているケー
     ブルを使う場合のみ．Serial-HOWTO の7章を参照のこと)
     CS8     : 8n1 (8 ビット，ノンパリティ，ストップビット 1)
     CLOCAL  : ローカル接続，モデム制御なし
     CREAD   : 受信文字(receiving characters)を有効にする．
     */
    newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
    
    /*
     IGNPAR  : パリティエラーのデータは無視する
     ICRNL   : CR を NL に対応させる(これを行わないと，他のコンピュータで 
     CR を入力しても，入力が終りにならない)
     それ以外の設定では，デバイスは raw モードである(他の入力処理は行わない)
     */
    newtio.c_iflag = IGNPAR | ICRNL;
    
    /*
     Raw モードでの出力
     */
    newtio.c_oflag = 0;
    
    /*
     ICANON  : カノニカル入力を有効にする
     全てのエコーを無効にし，プログラムに対してシグナルは送らせない
     */
    newtio.c_lflag = ICANON;
    
    /* 
     全ての制御文字を初期化する
     デフォルト値は /usr/include/termios.h を見れば分かるが，コメントに書
     いてあるので，ここでは見る必要はない．
     */
    newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
    newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
    newtio.c_cc[VERASE]   = 0;     /* del */
    newtio.c_cc[VKILL]    = 0;     /* @ */
    newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
    newtio.c_cc[VTIME]    = 0;     /* キャラクタ間タイマを使わない */
    newtio.c_cc[VMIN]     = 1;     /* 1文字来るまで，読み込みをブロックする */
    //newtio.c_cc[VSWTC]    = 0;     /* '\0' */
    newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
    newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
    newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
    newtio.c_cc[VEOL]     = 0;     /* '\0' */
    newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
    newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
    newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
    newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
    newtio.c_cc[VEOL2]    = 0;     /* '\0' */
    
    /* 
     モデムラインをクリアし，ポートの設定を有効にする
     */
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);
    
    /*
     端末の設定終了．入力を処理するできるようになった．
     */
    return true;
}

void Serial::threadStart()
{
    int result;

    if (!active) {
        active = 1;
        done = 0;

        // Create the server thread
        result = pthread_create(&thread, NULL, threadFunction, (void *)this);
        if (result) {
            fprintf(stderr,
                    "Failed to create thread: pthread_create(), %s",
                    strerror(result));
        }
    } 
}

void Serial::threadStop()
{
    int result;
    
    if (active) {
        // Signal thread to stop
        active = 0;
        
        // pthread_join waits for thread to terminate 
        // and then releases the thread's resources
        result = pthread_join( thread, NULL );
        if (result) {
            fprintf(stderr, "Failed to stop thread: pthread_join(), %s",
                    strerror(result));
        }
    }
}

void Serial::setCallback(void (*callback)(void *, int, void *), void *user_data) {
    cb = callback;
    ud = user_data;
}

void *Serial::threadFunction(void *data)
{
    int rs;
    Serial *s = (Serial *)data;

    while (s->active) {
        rs = read(s->fd, s->buf, 255);
        if (s->cb) s->cb(s->buf, rs, s->ud);
    }
    pthread_exit(NULL);
}

void Serial::writeData(void *data, int s)
{
	write(fd, data, s);
}

Serial::~Serial()
{
    threadStop();
    tcsetattr(fd,TCSANOW,&oldtio);
}




