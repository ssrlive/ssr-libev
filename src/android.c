/*
 * android.c - Setup IPC for shadowsocks-android
 *
 * Copyright (C) 2013 - 2016, Max Lv <max.c.lv@gmail.com>
 *
 * This file is part of the shadowsocks-libev.
 *
 * shadowsocks-libev is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-libev is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shadowsocks-libev; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#if ANDROID

#include <jni.h>
#include <assert.h>

#include <sys/un.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "netutils.h"
#include "utils.h"
#include "local.h"

#include <dlfcn.h>
#include <fake-dlfcn.h>
#include <assert.h>

int protect_socket(int fd) {
#define LIB_NETD_CLIENT_SO "libnetd_client.so"
    typedef int (*PFN_protectFromVpn)(int socketFd) ;
    static PFN_protectFromVpn protectFromVpn = NULL;
    if (protectFromVpn == NULL) {
        struct fake_dl_ctx *handle = fake_dlopen(SYSTEM_LIB_PATH LIB_NETD_CLIENT_SO, RTLD_NOW);
        if (!handle) {
            assert(!"cannot load " LIB_NETD_CLIENT_SO);
            return -1;
        }
        protectFromVpn = (PFN_protectFromVpn) fake_dlsym(handle, "protectFromVpn");
        fake_dlclose(handle);
        if (!protectFromVpn) {
            assert(!"required function protectFromVpn missing in " LIB_NETD_CLIENT_SO);
            return -1;
        }
        LOGI("%s", "==== protectFromVpn catched from " LIB_NETD_CLIENT_SO "! ====\n");
    }
    return protectFromVpn(fd);
}

extern char *stat_path;

int
send_traffic_stat(uint64_t tx, uint64_t rx)
{
    if (!stat_path) return 0;
    int sock;
    struct sockaddr_un addr;

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        LOGE("[android] socket() failed: %s (socket fd = %d)\n", strerror(errno), sock);
        return -1;
    }

    // Set timeout to 1s
    struct timeval tv;
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, stat_path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        LOGE("[android] connect() failed for stat_path: %s (socket fd = %d)\n",
             strerror(errno), sock);
        close(sock);
        return -1;
    }

    uint64_t stat[2] = { tx, rx };

    if (send(sock, stat, sizeof(stat), 0) == -1) {
        ERROR("[android] send");
        close(sock);
        return -1;
    }

    char ret = 0;

    if (recv(sock, &ret, 1, 0) == -1) {
        ERROR("[android] recv");
        close(sock);
        return -1;
    }

    close(sock);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_github_shadowsocks_bg_SsrClientWrapper_runSsrClient(JNIEnv *env, jclass clazz,
                                                               jobject cmd) {
    int result = -1;
    jclass alCls = NULL;
    do {
        alCls = (*env)->FindClass(env, "java/util/ArrayList");
        if (alCls == NULL) {
            break;
        }
        jmethodID alGetId = (*env)->GetMethodID(env, alCls, "get", "(I)Ljava/lang/Object;");
        jmethodID alSizeId = (*env)->GetMethodID(env, alCls, "size", "()I");
        if (alGetId == NULL || alSizeId == NULL) {
            break;
        }

        int arrayCount = (int) ((*env)->CallIntMethod(env, cmd, alSizeId));
        if (arrayCount <= 0) {
            break;
        }

        char ** argv = NULL;
        argv = (char **) calloc(arrayCount, sizeof(char*));
        if (argv == NULL) {
            break;
        }

        for (int index = 0; index < arrayCount; ++index) {
            jobject obj = (*env)->CallObjectMethod(env, cmd, alGetId, index);
            assert(obj);
            const char *cid = (*env)->GetStringUTFChars(env, obj, NULL);
            assert(cid);

            argv[index] = strdup(cid);
            assert(argv[index]);
            (*env)->DeleteLocalRef(env, obj);
        }

        result = main(arrayCount, argv);

        for (int index = 0; index < arrayCount; ++index) {
            free(argv[index]);
            argv[index] = NULL;
        }
        free(argv);
        argv = NULL;
    } while (false);

    if (alCls) {
        (*env)->DeleteLocalRef(env, alCls);
    }
    return result;
}

JNIEXPORT jint JNICALL
Java_com_github_shadowsocks_bg_SsrClientWrapper_stopSsrClient(JNIEnv *env, jclass clazz) {
    exit_main_event_loop();
    ev_sleep(1.3);
    return 0;
}

#endif // ANDROID
