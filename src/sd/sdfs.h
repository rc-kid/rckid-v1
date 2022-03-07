
#include "hw_config.h"
#include "sd/include/f_util.h"
#include "sd/ff14a/source/ff.h"

/** SD Filesystem driver. 
 */

class SDFS {
public:

    enum class OpenMode : BYTE {
        Read = FA_READ,
        Write = FA_WRITE,
        Append = FA_OPEN_APPEND,
    }; // SDFS::OpenMode

    /** RAII managed SDFS file. 
     */
    class File {
    public:

        File() = default;

        File(char const * path, OpenMode mode) {
            SDFS::check(f_open(& raw_, path, static_cast<BYTE>(mode)));
        }

        ~File() {
            close();
        }

        void close() {
            SDFS::check(f_close(& raw_));
        }

        size_t write(uint8_t const * buffer, size_t bytes) {
            UINT written = 0;
            check(f_write(& raw_, buffer, bytes, & written));
            return written;
        }

        size_t read(uint8_t * buffer, size_t maxSize) {
            UINT read = 0;
            check(f_read(& raw_, buffer, maxSize, & read));
            return read;
        }

    private:
        FIL raw_;

    }; // SDFS::File

    /** RAII managed SDFS folder. 
     */
    class Folder {

    }; // SDFS::Folder

    static bool mount() {
        sd_card_t *pSD = sd_get_by_num(0);
        return check(f_mount(&pSD->fatfs, pSD->pcName, 1));        
    }

    static bool unmount() {
        sd_card_t *pSD = sd_get_by_num(0);
        return check(f_unmount(pSD->pcName));        
    }

    static File open(char const * path, OpenMode mode) {
        return File{path, mode};
    } 

private:

    friend class File;
    friend class Folder;

    static bool check(FRESULT fr) {
        if (fr == 0) {
            return true;
        } else {
            printf("SD error %d: %s", fr, FRESULT_str(fr));
            return false;
        }
    }

}; // SDFS
