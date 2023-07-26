#pragma once

#include <filesystem>

#include "carousel.h"
#include "menu.h"

/** Carousel backed by a filesystem directory. 
 
    Show folders and files within. Useful for browsing the disk, can be used as a basic building block for file explorer. 
 */
class DirCarousel : public BaseCarousel {
public:

    DirCarousel(std::string const & root):
        root_{root} {
    }

protected:

    using DirEntry = std::filesystem::directory_entry;
    using DirIterator = std::filesystem::directory_iterator;
    using Path = std::filesystem::path;

    struct DirInfo {
        Path dir;
        std::vector<DirEntry> entries;

        DirInfo(Path dir):
            dir{dir} {
            for (auto const & entry : DirIterator{dir})
                entries.push_back(entry);
        }

        DirInfo(DirInfo const &) = delete;
        DirInfo(DirInfo &&) = default;
    };

    void reset() override {
        dirs_.clear();
        BaseCarousel::reset();
        enter(root_);
    }

    void itemSelected(size_t index) override {
        DirEntry & e = dirs_.back().entries[index];
        if (e.is_directory()) {
            enter(e.path());
        }
    }

    Item getItemFor(size_t index) override {
        DirEntry & e = dirs_.back().entries[index];
        if (e.is_directory())
            return Item{e.path().stem(), "assets/images/014-info.png"};
        else if (e.path().extension() == ".png") 
            return Item{e.path().stem(), e.path()};
        else
            return Item{e.path().stem()};
    }

    virtual void enter(Path path) {
        dirs_.emplace_back(path);
        BaseCarousel::enter(dirs_.back().entries.size());
    }

    void leave() override {
        ASSERT(dirs_.size() > 1);
        dirs_.pop_back();
        BaseCarousel::leave();
    }

    Path root_;

    std::vector<DirInfo> dirs_;

}; // DirCarousel

/** A very simple file explorer. 
 */
class FileExplorer : public DirCarousel {
public:
    FileExplorer(std::string const & root):
        DirCarousel{root},
        menu_{{"hello", "World", "foobar", "This is so good!"}} {
    }
protected:
    void setFooterHints() override  {
        DirCarousel::setFooterHints();
        window().addFooterItem(FooterItem::Select("Menu"));
    }

    void btnSelect(bool state) {
        if (state)
            window().showModal(&menu_);
    }

    Menu menu_;

}; 
