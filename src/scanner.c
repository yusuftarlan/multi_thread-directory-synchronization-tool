
#include "queue.h"
void scan_directory(const char *root_src, const char *root_dst, TaskQueue *task_q)
{
    DirQueue dq;
    dir_queue_init(&dq);

    // BFS Başlangıcı: Kök dizinleri kuyruğa at
    dir_queue_push(&dq, root_src, root_dst);

    char current_src[PATH_MAX];
    char current_dst[PATH_MAX];

    // Kuyrukta klasör olduğu sürece dön (BFS Döngüsü)
    while (dir_queue_pop(&dq, current_src, current_dst))
    {
        DIR *dir = opendir(current_src);
        if (!dir)
        {
            perror("Dizin acilamadi");
            continue;
        }

        struct dirent *entry;
        struct stat src_stat, dest_stat;

        while ((entry = readdir(dir)) != NULL)
        {
            // . ve .. dizinlerini atla
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char src_path[PATH_MAX];
            char dest_path[PATH_MAX];

            snprintf(src_path, PATH_MAX, "%s/%s", current_src, entry->d_name);
            snprintf(dest_path, PATH_MAX, "%s/%s", current_dst, entry->d_name);

            // Sembolik link tehlikesine karşı lstat
            if (lstat(src_path, &src_stat) == -1)
                continue;

            if (S_ISLNK(src_stat.st_mode))
                continue; // Linkleri atla

            // EĞER KLASÖR İSE: Hedefte oluştur ve kuyruğa at
            if (S_ISDIR(src_stat.st_mode))
            {
                mkdir(dest_path, src_stat.st_mode & 0777);
                dir_queue_push(&dq, src_path, dest_path); // İleride taranmak üzere listeye eklendi
            }
            // EĞER DOSYA İSE: Kontrol et ve Thread Pool'a (TaskQueue) yolla
            else if (S_ISREG(src_stat.st_mode))
            {
                int need_copy = 0;

                if (stat(dest_path, &dest_stat) == -1)
                {
                    need_copy = 1; // Yoksa kopyala
                }
                else if (src_stat.st_mtime > dest_stat.st_mtime || src_stat.st_size != dest_stat.st_size)
                {
                    need_copy = 1; // Değişmişse kopyala
                }

                if (need_copy)
                {
                    CopyTask task;
                    strncpy(task.source_path, src_path, PATH_MAX);
                    task.source_path[PATH_MAX - 1] = '\0';

                    strncpy(task.dest_path, dest_path, PATH_MAX);
                    task.dest_path[PATH_MAX - 1] = '\0';

                    printf("[Scanner] Dosya kopyalama kuyruguna alindi: %s\n", src_path);

                    // İşçi thread'leri uyandırmak için ana iş kuyruğuna (Mutex'li olan) ekle
                    queue_push(task_q, task);
                }
            }
        }
        closedir(dir);
    }
}