#ifndef UTIL_H
#define UTIL_H

char* util_get_config_dir(void);
char* util_get_cache_dir(void);
void util_create_dir_if_not_exists(const char* dirpath);
void util_create_file_if_not_exists(const char* filename);

#endif /* end of include guard: UTIL_H */
