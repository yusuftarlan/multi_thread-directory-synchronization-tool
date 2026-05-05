#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h> // mkdir için gerekli kütüphane
#include "queue.h"

#define THREAD_COUNT 4 // Kaç thread çalışacağını buradan belirliyoruz

void scan_directory(const char *src_dir, const char *dest_dir, TaskQueue *q);
void *worker_thread(void *arg);

TaskQueue file_queue; // Global kuyruğumuz

int main(int argc, char *argv[])
{
    // 1. Argüman Kontrolü
    if (argc != 3)
    {
        fprintf(stderr, "Kullanim: %s <kaynak_dizin> <hedef_dizin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[1];
    const char *dest_dir = argv[2];

    // 2. Kuyruğu Başlat ve Hedef Klasörü Oluştur
    queue_init(&file_queue);
    mkdir(dest_dir, 0755);

    // 3. Tüketicileri (Worker Thread'leri) Uyandır
    pthread_t workers[THREAD_COUNT]; // Sadece bir tane thread dizisi tanımlıyoruz
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        // Thread'lere argüman olarak &file_queue adresini gönderiyoruz (NULL yerine)
        if (pthread_create(&workers[i], NULL, worker_thread, &file_queue) != 0)
        {
            perror("Worker thread olusturulamadi");
            return EXIT_FAILURE;
        }
    }

    printf("--- Tarama Basliyor ---\n");

    // 4. Ana thread'i Scanner olarak çalıştırıyoruz
    // Sabit stringler yerine kullanıcının girdiği dizinleri gönderiyoruz
    scan_directory(src_dir, dest_dir, &file_queue);

    // 5. Tarama bittiğinde kuyruğu kapat ve işçilere "paydos" de
    // Kapanış bayrağını mutlaka MUTEX KİLİDİ İÇİNDE değiştirmeliyiz
    pthread_mutex_lock(&file_queue.lock);
    file_queue.shutdown = 1;
    pthread_cond_broadcast(&file_queue.not_empty); // Bekleyen tüm işçileri uyandır
    pthread_mutex_unlock(&file_queue.lock);

    // 6. Thread'lerin işlerini bitirmesini bekle
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(workers[i], NULL); // workers dizisini bekliyoruz
    }

    // 7. Temizlik Aşaması (Sistem kaynaklarını iade et)
    pthread_mutex_destroy(&file_queue.lock);
    pthread_cond_destroy(&file_queue.not_empty);
    pthread_cond_destroy(&file_queue.not_full);

    printf("--- Tüm İşlemler Başarıyla Tamamlandı ---\n");
    return 0;
}