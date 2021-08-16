#include "ppos_disk.h"
#include "ppos_data.h"
#include "ppos-core-globals.h"
#include "disk.h"

disk_t disk;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) {
    int result = disk_cmd(DISK_CMD_INIT, 0, NULL);

    // retorna -1 se houver erro na inicializacao
    if (result < 0) {
        return -1;
    }

    result = disk_cmd(DISK_CMD_DISKSIZE, 0, NULL);
    if (result < 0) {
        return -1;
    }
    *numBlocks = result;

    result = disk_cmd(DISK_CMD_BLOCKSIZE, 0, NULL);
    if (result < 0) {
        return -1;
    }
    *blockSize = result;
    disk.suspendedQueue = NULL;

    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) {
    // TODO: coloca na suspendedQueue

    int result = disk_cmd(DISK_CMD_READ, block, buffer);
    if (result < 0) {
        return -1;
    }

    result = disk_cmd(DISK_CMD_STATUS, 0, 0);
    while (result != DISK_STATUS_IDLE) {
        if (result == DISK_STATUS_UNKNOWN) {
            return -1;
        }
        result = disk_cmd(DISK_CMD_STATUS, 0, 0);
    }

    // TODO: retira da suspendedQueue

    return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) {
    // TODO: coloca na suspendedQueue

    int result = disk_cmd(DISK_CMD_WRITE, block, buffer);
    if (result < 0) {
        return -1;
    }

    result = disk_cmd(DISK_CMD_STATUS, 0, 0);
    while (result != DISK_STATUS_IDLE) {
        if (result == DISK_STATUS_UNKNOWN) {
            return -1;
        }
        result = disk_cmd(DISK_CMD_STATUS, 0, 0);
    }
    
    // TODO: etira da suspendedQueue


    return 0;
}