/* this file contains configuration for the file system. */

#include "berry.h"
#include <string.h>
#include <stdlib.h>
#include <rtthread.h>

#ifdef UNUSED
  #undef UNUSED
#endif
#define UNUSED(__arg) (void)(__arg)

/* exit and abort */

void be_rtt_abort(void)
{
}

void be_rtt_exit(int code)
{
    UNUSED(code);
}

/* standard input and output */

void be_writebuffer(const char *buffer, size_t length)
{
    rt_device_write(rt_console_get_device(), 0, buffer, length);
}

char* be_readstring(char* buffer, size_t size)
{
    return NULL;
}

void* be_fopen(const char *filename, const char *modes)
{
#if defined(RT_USING_DFS)
    return fopen(filename, modes);
#else
    UNUSED(filename), UNUSED(modes);
    return NULL;
#endif
}

int be_fclose(void *hfile)
{
#if defined(RT_USING_DFS)
    return fclose(hfile);
#else
    UNUSED(hfile);
    return 0;
#endif
}

size_t be_fwrite(void *hfile, const void *buffer, size_t length)
{
#if defined(RT_USING_DFS)
    return fwrite(buffer, 1, length, hfile);
#else
    UNUSED(hfile), UNUSED(buffer), UNUSED(length);
    return 0;
#endif
}

size_t be_fread(void *hfile, void *buffer, size_t length)
{
#if defined(RT_USING_DFS)
    return fread(buffer, 1, length, hfile);
#else
    UNUSED(hfile), UNUSED(buffer), UNUSED(length);
    return 0;
#endif
}

char* be_fgets(void *hfile, void *buffer, int size)
{
#if defined(RT_USING_DFS)
    return fgets(buffer, size, hfile);
#else
    UNUSED(hfile), UNUSED(buffer), UNUSED(size);
    return NULL;
#endif
}

int be_fseek(void *hfile, long offset)
{
#if defined(RT_USING_DFS)
    return fseek(hfile, offset, SEEK_SET);
#else
    UNUSED(hfile), UNUSED(offset);
    return 0;
#endif
}

long int be_ftell(void *hfile)
{
#if defined(RT_USING_DFS)
    return ftell(hfile);
#else
    UNUSED(hfile);
    return 0;
#endif
}

long int be_fflush(void *hfile)
{
#if defined(RT_USING_DFS)
    return fflush(hfile);
#else
    UNUSED(hfile);
    return 0;
#endif
}

size_t be_fsize(void *hfile)
{
#if defined(RT_USING_DFS)
    long int size, offset = be_ftell(hfile);
    fseek(hfile, 0L, SEEK_END);
    size = ftell(hfile);
    fseek(hfile, offset, SEEK_SET);
    return size;
#else
    UNUSED(hfile);
    return 0;
#endif
}

#if BE_USE_OS_MODULE
#include <dfs_posix.h>

int be_isdir(const char *path)
{
    struct stat path_stat;
    int res = stat(path, &path_stat);
    return res == 0 && S_ISDIR(path_stat.st_mode);
}

int be_isfile(const char *path)
{
    struct stat path_stat;
    int res = stat(path, &path_stat);
    return res == 0 && !S_ISDIR(path_stat.st_mode);
}

int be_isexist(const char *path)
{
    struct stat path_stat;
    return stat(path, &path_stat) == 0;
}

char* be_getcwd(char *buf, size_t size)
{
    return getcwd(buf, size);
}

int be_chdir(const char *path)
{
    return chdir(path);
}

int be_mkdir(const char *path)
{
    return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

int be_unlink(const char *filename)
{
    return remove(filename);
}

int be_dirfirst(bdirinfo *info, const char *path)
{
    info->dir = opendir(path);
    if (info->dir) {
        return be_dirnext(info);
    }
    return 1;
}

int be_dirnext(bdirinfo *info)
{
    struct dirent *file;
    info->file = file = readdir(info->dir);
    if (file) {
        info->name = file->d_name;
        return 0;
    }
    return 1;
}

int be_dirclose(bdirinfo *info)
{
    return closedir(info->dir) != 0;
}

#endif /* POSIX */
