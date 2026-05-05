#include "common.h"
#include "queue.h"

#define BUFFER_SIZE 4096 // Hocanın istediği blok boyutu (4KB)

void *worker_thread(void *arg)
{
    TaskQueue *q = (TaskQueue *)arg;
    CopyTask task;

    while (queue_pop(q, &task))
    {
        // queue_pop() başarılı olduğu sürece döner (0 dönerse dükkan kapandı demektir)
        printf("[Worker] Kopyalaniyor: %s\n", task.source_path);

        int src_fd = open(task.source_path, O_RDONLY);
        if (src_fd < 0)
        {
            perror("Kaynak dosya acilamadi");
            continue;
        }

        int dest_fd = open(task.dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dest_fd < 0)
        {
            perror("Hedef dosya acilamadi");
            close(src_fd); // AÇIK KALAN KAYNAĞI KAPAT! (FD Leak Çözümü)
            continue;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read, bytes_written;

        // Blok blok okuma
        while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0)
        {
            char *out_ptr = buffer;
            ssize_t bytes_to_write = bytes_read;

            // Kısmi yazma (Partial Write) Çözümü: Tüm baytlar yazılana kadar dön
            while (bytes_to_write > 0)
            {
                bytes_written = write(dest_fd, out_ptr, bytes_to_write);

                if (bytes_written < 0)
                {
                    perror("Yazma hatasi");
                    goto cleanup; // İşlemi iptal et ve dosyaları kapatmaya git
                }

                bytes_to_write -= bytes_written; // Kalan yazılacak bayt miktarını azalt
                out_ptr += bytes_written;        // Buffer'daki okuma işaretçisini kaydır
            }
        }

        if (bytes_read < 0)
        {
            perror("Okuma hatasi");
        }

        // Başarılı kopyalama sonu logu
        printf("[Worker] Tamamlandi: %s\n", task.dest_path);

    cleanup:
        // Her halükarda dosyaları kapat (Resource Management)
        close(src_fd);
        close(dest_fd);
    }
    return NULL;
}
