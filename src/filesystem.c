#include "src/br_filesystem.h"
#include "src/br_pp.h"
#include "src/br_str.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(__MINGW32__)
#  include "src/desktop/linux/filesystem.c"
#elif defined(_WIN32) || defined(__CYGWIN__)
#  include "src/desktop/win/filesystem.c"
#elif defined(__EMSCRIPTEN__)
#  include "src/web/filesystem.c"
#else
#  error "Unsupported Platform"
#endif

static bool br_permastate_get_home_dir(br_str_t* home) {
#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
  return br_str_push_c_str(home, getenv("HOME"));
#elif defined(_WIN32) || defined(__CYGWIN__)
  return br_str_push_c_str(home, getenv("LOCALAPPDATA"));
#else
  (void)home;
  return false;
#endif
}

bool br_fs_get_config_dir(br_str_t* path) {
  if (false == br_permastate_get_home_dir(path)) return false;;
#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
  return br_fs_cd(path, br_strv_from_literal(".config/brplot"));
#elif defined(_WIN32) || defined(__CYGWIN__)
  return br_fs_cd(path, br_strv_from_literal("brplot"));
#else
  (void)path;
  return false;
#endif
}

#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
bool br_fs_mkdir(br_strv_t path) {
  if (br_fs_exists(path)) return true;
  char* scrach = br_strv_to_scrach(path);
#  if !defined(ACCESSPERMS)
#    define ACCESSPERMS (S_IRWXU|S_IRWXG|S_IRWXO) /* 0777 */
#  endif

  int ret = mkdir(scrach, ACCESSPERMS);
  if (ret == -1) LOGE("Failed to create directory `%s`: %s", scrach, strerror(errno));
  br_scrach_free();
  return ret == 0;
}

bool br_fs_exists(br_strv_t path) {
  struct stat s = { 0 };
  char* scrach = br_strv_to_scrach(path);
  stat(scrach, &s);
  br_scrach_free();
  return 0 != ((s.st_mode & S_IFMT) & (S_IFDIR | S_IFCHR | S_IFBLK | S_IFREG));
}
#elif defined(__MINGW32__)
bool br_fs_mkdir(br_strv_t path) { (void)path; return false; }
bool br_fs_exists(br_strv_t path) { (void)path; return false; }
#elif defined(_WIN32) || defined(__CYGWIN__)
#elif defined(__EMSCRIPTEN__)
bool br_fs_mkdir(br_strv_t path) { (void)path; return false; }
bool br_fs_exists(br_strv_t path) { (void)path; return false; }
#endif

static const uint32_t GCrc32LookupTable[256] =
{
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D,
};

uint32_t br_fs_crc(const void* data_p, size_t data_size, uint32_t seed) // Stolen from ImGui
{
    uint32_t crc = ~seed;
    const unsigned char* data = (const unsigned char*)data_p;
    while (data_size-- != 0)
        crc = (crc >> 8) ^ GCrc32LookupTable[(crc & 0xFF) ^ *data++];
    return ~crc;
}

bool br_fs_move(const char* from, const char* to) {
  if (0 == rename(from, to)) return true;
  LOGE("Failed to move file %s to %s: %s", from, to, strerror(errno));
  return false;
}

bool br_fs_read(const char* path, br_str_t* out_content) {
  FILE* file        = NULL;
  long size         = 0;
  size_t wanted_cap = 0;
  bool success      = true;

  if (NULL == path)                                                          BR_ERROR("Provided path is NULL");
  if (NULL == (file = fopen(path, "rb")))                                    BR_ERROR("Failed to open file %s: %s", path, strerror(errno));
  if (-1 == fseek(file, 0, SEEK_END))                                        BR_ERROR("Failed to seek file %s: %s", path, strerror(errno));
  if (-1 == (size = ftell(file)))                                            BR_ERROR("Failed to get the file size %s: %s", path, strerror(errno));
  if (-1 == fseek(file, 0, SEEK_SET))                                        BR_ERROR("Failed to get seek set file %s: %s", path, strerror(errno));
  out_content->len = 0;
  wanted_cap = (size_t)size + 1;
  if (false == br_str_push_uninitialized(out_content, (uint32_t)wanted_cap)) BR_ERROR("Failed to push unininitialized string");
  if (out_content->cap < wanted_cap)                                         BR_ERROR("Failed to malloc %zu bytes file %s: %s", wanted_cap, path, strerror(errno));
  if (size != (long)fread(out_content->str, 1, (size_t)size, file))          BR_ERROR("Failed to read file %s: %s", path, strerror(errno));
  out_content->str[size] = '\0';
  out_content->len = (uint32_t)size;
  goto done;

error:
  success = false;
  out_content->len = 0;

done:
  if (file != NULL) fclose(file);
  return success;
}

br_str_t br_fs_read1(const char* path) {
  br_str_t str = { 0 };
  br_fs_read(path, &str);
  return str;
}

bool br_fs_list_dir(br_strv_t path, br_fs_files_t* out_files) {
  DIR* dir = NULL;
  bool success = true;
  struct dirent *de = NULL;
  size_t i = 0;
  br_fs_file_t* s = NULL;

  out_files->cur_dir.len = 0;
  br_str_push_strv(&out_files->cur_dir, path);
  br_str_push_zero(&out_files->cur_dir);
  if (NULL == (dir = opendir(out_files->cur_dir.str))) goto error;
  br_str_copy2(&out_files->last_good_dir, out_files->cur_dir);
  while (NULL != (de = readdir(dir))) {
    if (strcmp(".", de->d_name) == 0) continue;
    if (strcmp("..", de->d_name) == 0) continue;
    if (out_files->len <= i) br_da_push(*out_files, ((br_fs_file_t) {0}));
    s = br_da_getp(*out_files, i);
    s->name.len = 0;
    br_str_push_c_str(&s->name, de->d_name);
#if defined(_DIRENT_HAVE_D_TYPE)
    switch (de->d_type) {
      case DT_DIR: s->kind = br_fs_file_kind_dir; break;
      case DT_REG: s->kind = br_fs_file_kind_file; break;
      default: s->kind = br_fs_file_kind_unknown; break;
    };
#else
    struct stat st = { 0 };
    stat(de->d_name, &st);
    switch (st.st_mode & S_IFMT) {
      case S_IFDIR: s->kind = br_fs_file_kind_dir; break;
      case S_IFREG: s->kind = br_fs_file_kind_file; break;
      default: s->kind = br_fs_file_kind_unknown; break;
    }
#endif
    ++i;
  }
  out_files->real_len = i;
  qsort(out_files->arr, out_files->real_len, sizeof(out_files->arr[0]), br_fs_files_sort);
  goto done;

error:
  success = false;

done:
  if (NULL != dir) closedir(dir);
  return success;
}

