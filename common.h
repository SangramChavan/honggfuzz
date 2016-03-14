/*
 *
 * honggfuzz - core structures and macros
 * -----------------------------------------
 *
 * Author: Robert Swiecki <swiecki@google.com>
 *
 * Copyright 2010-2015 by Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 */

#ifndef _HF_COMMON_H_
#define _HF_COMMON_H_

#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/param.h>
#include <sys/types.h>

#ifdef __clang__
#include <stdatomic.h>
#endif

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#define PROG_NAME "honggfuzz"
#define PROG_VERSION "0.6rc"
#define PROG_AUTHORS "Robert Swiecki <swiecki@google.com> et al.,\nCopyright 2010-2015 by Google Inc. All Rights Reserved."

/* Go-style defer implementation */
#define __STRMERGE(a, b) a##b
#define _STRMERGE(a, b) __STRMERGE(a, b)

#ifdef __clang__
static void __attribute__ ((unused)) _clang_cleanup_func(void (^*dfunc) (void))
{
    (*dfunc) ();
}

#define DEFER(a) void (^_STRMERGE(__defer_f_, __COUNTER__))(void) __attribute__((cleanup(_clang_cleanup_func))) __attribute__((unused)) = ^{ a; }
#else
#define __block
#define _DEFER(a, count) void _STRMERGE(__defer_f_, count)(void *_defer_arg __attribute__((unused))) { a; } ; \
    int _STRMERGE(_defer_var_, count) __attribute__((cleanup(_STRMERGE(__defer_f_, count)))) __attribute__((unused))
#define DEFER(a) _DEFER(a, __COUNTER__)
#endif

/* Name of the template which will be replaced with the proper name of the file */
#define _HF_FILE_PLACEHOLDER "___FILE___"

/* Default name of the report created with some architectures */
#define _HF_REPORT_FILE "HONGGFUZZ.REPORT.TXT"

/* Default stack-size of created threads. Must be bigger then _HF_DYNAMIC_FILE_MAX_SZ */
#define _HF_PTHREAD_STACKSIZE (1024 * 1024 * 8) /* 8MB */

/* Align to the upper-page boundary */
#define _HF_PAGE_ALIGN_UP(x)  (((size_t)x + (size_t)getpagesize() - (size_t)1) & ~((size_t)getpagesize() - (size_t)1))

/* String buffer size for function names in stack traces produced from libunwind */
#define _HF_FUNC_NAME_SZ    256 // Should be alright for mangled C++ procs too

/* Number of crash verifier iterations before tag crash as stable */
#define _HF_VERIFIER_ITER   5

/* Constant prefix used for single frame crashes stackhash masking */
#define _HF_SINGLE_FRAME_MASK  0xBADBAD0000000000

/* Size (in bytes) for report data to be stored in stack before written to file */
#define _HF_REPORT_SIZE 8192

#define _HF_DYNFILE_SUB_MASK 0xFFFUL    // Zero-set two MSB

/* Bitmap size */
#define _HF_BITMAP_SIZE 0x2AFFFFF

/* Directory in workspace to store sanitizer coverage data */
#define _HF_SANCOV_DIR "HF_SANCOV"

#if defined(__ANDROID__)
#define _HF_MONITOR_SIGABRT 0
#else
#define _HF_MONITOR_SIGABRT 1
#endif

/* Size of remote pid cmdline char buffer */
#define _HF_PROC_CMDLINE_SZ 8192

typedef enum {
    _HF_DYNFILE_NONE = 0x0,
    _HF_DYNFILE_INSTR_COUNT = 0x1,
    _HF_DYNFILE_BRANCH_COUNT = 0x2,
    _HF_DYNFILE_BTS_BLOCK = 0x8,
    _HF_DYNFILE_BTS_EDGE = 0x10,
    _HF_DYNFILE_IPT_BLOCK = 0x20,
    _HF_DYNFILE_CUSTOM = 0x40,
} dynFileMethod_t;

typedef struct {
    uint64_t cpuInstrCnt;
    uint64_t cpuBranchCnt;
    uint64_t cpuBtsBlockCnt;
    uint64_t cpuBtsEdgeCnt;
    uint64_t cpuIptBlockCnt;
    uint64_t customCnt;
} hwcnt_t;

/* Sanitizer coverage specific data structures */
typedef struct {
    uint64_t hitBBCnt;
    uint64_t totalBBCnt;
    uint64_t dsoCnt;
    uint64_t iDsoCnt;
    uint64_t newBBCnt;
    uint64_t crashesCnt;
} sancovcnt_t;

typedef struct {
    uint32_t capacity;
    uint32_t *pChunks;
    uint32_t nChunks;
} bitmap_t;

/* Memory map struct */
typedef struct __attribute__ ((packed)) {
    uint64_t start;             // region start addr
    uint64_t end;               // region end addr
    uint64_t base;              // region base addr
    char mapName[NAME_MAX];     // bin/DSO name
    uint64_t bbCnt;
    uint64_t newBBCnt;
} memMap_t;

/* Trie node data struct */
typedef struct __attribute__ ((packed)) {
    bitmap_t *pBM;
} trieData_t;

/* Trie node struct */
typedef struct __attribute__ ((packed)) node {
    char key;
    trieData_t data;
    struct node *next;
    struct node *prev;
    struct node *children;
    struct node *parent;
} node_t;

/* EOF Sanitizer coverage specific data structures */

typedef struct {
    char *asanOpts;
    char *msanOpts;
    char *ubsanOpts;
} sanOpts_t;

typedef enum {
    _HF_STATE_UNSET = 0,
    _HF_STATE_STATIC = 1,
    _HF_STATE_DYNAMIC_PRE = 2,
    _HF_STATE_DYNAMIC_MAIN = 3,
} fuzzState_t;

typedef struct {
    char **cmdline;
    char cmdline_txt[PATH_MAX];
    char *inputFile;
    bool nullifyStdio;
    bool fuzzStdin;
    bool saveUnique;
    bool useScreen;
    bool useVerifier;
    time_t timeStart;
    char *fileExtn;
    char *workDir;
    double origFlipRate;
    char *externalCommand;
    const char *dictionaryFile;
    char **dictionary;
    const char *blacklistFile;
    uint64_t *blacklist;
    size_t blacklistCnt;
    long tmOut;
    size_t dictionaryCnt;
    size_t mutationsMax;
    size_t threadsMax;
    size_t threadsFinished;
    size_t maxFileSz;
    char *reportFile;
    uint64_t asLimit;
    char **files;
    size_t fileCnt;
    size_t lastCheckedFileIndex;
    int exeFd;
    bool clearEnv;
    char *envs[128];

    fuzzState_t state;

    size_t mutationsCnt;
    size_t crashesCnt;
    size_t uniqueCrashesCnt;
    size_t verifiedCrashesCnt;
    size_t blCrashesCnt;
    size_t timeoutedCnt;

    uint8_t *dynamicFileBest;
    size_t dynamicFileBestSz;
    dynFileMethod_t dynFileMethod;
    sancovcnt_t sanCovCnts;
    pthread_mutex_t dynamicFile_mutex;
    pthread_mutex_t sanCov_mutex;
    sanOpts_t sanOpts;
    size_t dynFileIterExpire;
    bool useSanCov;
    node_t *covMetadata;
    bool clearCovMetadata;

    /* For the Linux code */
    hwcnt_t hwCnts;
    uint64_t dynamicCutOffAddr;
    bool disableRandomization;
    bool msanReportUMRS;
    void *ignoreAddr;
    size_t numMajorFrames;
    pid_t pid;
    const char *pidFile;
    char *pidCmd;
} honggfuzz_t;

typedef struct fuzzer_t {
    pid_t pid;
    int64_t timeStartedMillis;
    char origFileName[PATH_MAX];
    char fileName[PATH_MAX];
    char crashFileName[PATH_MAX];
    uint64_t pc;
    uint64_t backtrace;
    uint64_t access;
    int exception;
    char report[_HF_REPORT_SIZE];
    bool mainWorker;
    float flipRate;

    sancovcnt_t sanCovCnts;
    uint8_t *dynamicFile;
    size_t dynamicFileSz;

    /* For Linux code */
    hwcnt_t hwCnts;
} fuzzer_t;

#define _HF_MAX_FUNCS 80
typedef struct {
    void *pc;
    char func[_HF_FUNC_NAME_SZ];
    size_t line;
} funcs_t;

#define ARRAYSIZE(x) (sizeof(x) / sizeof(*x))

#define rmb()	__asm__ __volatile__("":::"memory")
#define wmb()	__sync_synchronize()

#endif
