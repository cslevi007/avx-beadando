#include <stdio.h>
#include <immintrin.h>

#define SPACE1_SIZE 1080054 // A space2.bmp mérete 600 * 600 * 3 + 54, mert 600*600 pixelből áll és mindegyiknek pixelnek RGB kompnensei vannak és ehhez még hozzáadódik a header mérete (54)
#define SPACE2_SIZE 4320054 // A space2.bmp mérete 1200 * 1200 * 3, mert 1200*1200 pixelből áll és mindegyik pixelnek RGB komponensei vannak és ehhez még hozzáadódik a header mérete (54)
#define MODIFIED_SIZE 2160054 // Ez az átalakítás során keletkezett köztes kép mérete 600 * 1200 * 3 + 54

int main()
{
    FILE *originalSpace2 = fopen("/space_2.bmp", "rb"); // bmp file megnyitása csak olvasásra, bináris mód
    FILE *originalSpace1 = fopen("/space_1.bmp", "rb"); // bmp file megnyitása csak olvasásra, bináris mód

    // A szükséges tömbök lefoglalása a konstansokkal.
    char *space2Array = (char*) malloc((SPACE2_SIZE)*sizeof (char));
    char *space1Array = (char*) malloc((SPACE1_SIZE)*sizeof (char));
    char *verticallyDownscaledArray = (char*) malloc((MODIFIED_SIZE)*sizeof (char)); // Tömb a 1200*600-as képnek.
    char *downscaledArray = (char*) malloc((SPACE1_SIZE)*sizeof (char));

    //A feladat - bitmapek betöltése
    // Felolvassa az adatot az utolsó paraméterként megadott streamből az első paraméterként megadott tömbbe
    // Második paraméter az egyes beolvasandó elemek mérete bájtokban.
    // Harmadik paraméter az elemek száma, amelyek mindegyike bájt méretű.
    fread(space2Array, SPACE2_SIZE, 1, originalSpace2);
    fread(space1Array, SPACE1_SIZE, 1, originalSpace1);

    // A beolvasés végén a pointer a fájl végén maradt, ezért ez visszaállításra kerül a fájl elejére
    // A stream fájlpozícióját a megadott eltolásra állítja.
    // Az eltolás a SEEK_SET miatt a fájl elejéről kezdődik.
    fseek(originalSpace1, 0, SEEK_SET);
    fread(downscaledArray, 54, 1, originalSpace1);

    // Képek bezárása
    fclose(originalSpace2);
    fclose(originalSpace1);


    //B feladat - vertikális és horizontális módosítás
    // A feladat megoldásához használt ciklus változók
    int i, j, k = 54; // k változó az új kép tárolásához használatos

    // Kép vertikális kicsinyítése, egymás alatti sorok eleminek átlagolásával
    // Egyszerre két sort kell ugrani, ezért növelődik 7200-al a külső ciklusváltozó (egy sor 1200 * 3 => 3600)
    for(i = 54; i < SPACE2_SIZE; i += 7200) {
        // Azért 32-essével haladunk mert ennyi bájtot lehet betölteni a 256 bites regiszterekbe
        for(j = i; j < i + 3584; j += 32, k += 32){ // A j azért i + 3584-ig megy, mert 32 bájtosával betöltve az adatot ennyit lehet teljesen betölteni (112 * 32 = 3584), marad a végén 16 bájt, ami utána külön van kezelve egy 128 bites (16 bájtos regiszterben) 
            __m256i mm_x = _mm256_loadu_si256((__m256i *)&(space2Array[j]));
            __m256i mm_y = _mm256_loadu_si256((__m256i *)&(space2Array[j + 3600]));

            // epu8 miatt 8 bitenként fogja elvgézeni az átlagolást (a pixelek 8 bitesek)
            __m256i mm_avg = _mm256_avg_epu8(mm_x, mm_y);
            _mm256_storeu_si256((__m256i *)&(verticallyDownscaledArray[k]), mm_avg);
        }
        __m128i mm_top= _mm_loadu_si128((__m128i *)&(space2Array[j]));
        __m128i mm_bottom = _mm_loadu_si128((__m128i *)&(space2Array[j + 3600]));
        __m128i mm_avg = _mm_avg_epu8(mm_top, mm_bottom);
        _mm_storeu_si128((__m128i *)&(verticallyDownscaledArray[k]), mm_avg);
        k += 16;
    }

    // Horizontális kicsinyítés.
    // Az i hatot nő a j pedig csak 3-at, mert csak minden masodik pixeladatot kell megtartani (egy pixel pixel 3 bájt)
    for(i = 54, j = 54; i < MODIFIED_SIZE; i += 6, j += 3){
        downscaledArray[j] = verticallyDownscaledArray[i];
        downscaledArray[j + 1] = verticallyDownscaledArray[i + 1];
        downscaledArray[j + 2] = verticallyDownscaledArray[i + 2];
    }


    // C feladat 
    // space_1.bmp és az átméretezett space_2.bmp minden pixelértékének átlagolása
    for(i = 54; i < SPACE1_SIZE; i += 32) {
        __m256i mm_x = _mm256_loadu_si256((__m256i *)&(downscaledArray[i]));
        __m256i mm_y = _mm256_loadu_si256((__m256i *)&(space1Array[i]));
        __m256i mm_avg = _mm256_avg_epu8(mm_x, mm_y);
        _mm256_storeu_si256((__m256i *)&(downscaledArray[i]), mm_avg); 
    }

    // Új bmp fájl létrehozásra írásra és olvasásra, bináirs mód
    FILE *result = fopen("space_new.bmp", "wb+");
    // Az átlagolás eredményének beleírása az új képbe
    fwrite(downscaledArray, 1, SPACE1_SIZE, result);

    // Eredmény fájl bezárása
    fclose(result);

    // A feladat során lefoglalt memóriaterületek felszabadítása
    free(space2Array);
    free(space1Array);
    free(verticallyDownscaledArray);
    free(downscaledArray);
}