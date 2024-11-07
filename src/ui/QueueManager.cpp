#include "QueueManager.h"
#include <memory>

QM::QueueManager::QueueManager(wxEvtHandler* eventHandler, std::string jobsdir) {
    // need to send events into the mainwindow by the threads...
    this->eventHandler = eventHandler;
    this->eventHandler->Bind(wxEVT_THREAD, &QueueManager::OnThreadMessage, this);
    this->jobsDir   = jobsdir;
    this->QueueList = std::map<int, std::shared_ptr<QM::QueueItem>>();
    this->LoadJobListFromDir();
}

QM::QueueManager::~QueueManager() {
    if (this->currentItem) {
        this->currentItem = nullptr;
    }
}

int QM::QueueManager::AddItem(const QM::QueueItem& _item, bool fromFile) {
    std::lock_guard<std::mutex> lock(queueMutex);
    std::shared_ptr<QM::QueueItem> item = std::make_shared<QM::QueueItem>(_item);
    if (item->id == 0) {
        item->id = this->GetAnId();
    }
    if (item->created_at == 0) {
        item->created_at = this->GetCurrentUnixTimestamp();
    }
    if (item->id == this->lastExtId) {
        return 0;
    }

    this->lastExtId = item->id;

    this->QueueList[item->id] = item;

    if (fromFile == false) {
        this->SaveJobToFile(*item);
    }

    this->SendEventToMainWindow(QM::QueueEvents::ITEM_ADDED, item);
    if (this->isRunning == false && item->status == QM::QueueStatus::PENDING) {
        this->currentItem = item;
        this->SendEventToMainWindow(QM::QueueEvents::ITEM_START, item);
        this->isRunning = true;
    }

    return item->id;
}
void QM::QueueManager::UpdateItem(const QM::QueueItem& item) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (this->QueueList.find(item.id) != this->QueueList.end()) {
        *this->QueueList[item.id] = item;
        this->SaveJobToFile(item);
    }
}

std::shared_ptr<QM::QueueItem> QM::QueueManager::GetItemPtr(int id) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (this->QueueList.find(id) == this->QueueList.end()) {
        return nullptr;
    } else {
        return this->QueueList[id];
    }
}

std::shared_ptr<QM::QueueItem> QM::QueueManager::GetItemPtr(const QM::QueueItem& item) {
    return this->GetItemPtr(item.id);
}

const std::map<int, std::shared_ptr<QM::QueueItem>> QM::QueueManager::getList() {
    std::lock_guard<std::mutex> lock(queueMutex);
    std::map<int, std::shared_ptr<QM::QueueItem>> newlist;
    for (auto item : this->QueueList) {
        newlist[item.first] = item.second;
    }
    return newlist;
}

std::shared_ptr<QM::QueueItem> QM::QueueManager::Duplicate(std::shared_ptr<QM::QueueItem> item) {
    if (this->QueueList.find(item->id) == this->QueueList.end()) {
        return nullptr;
    }

    std::shared_ptr<QM::QueueItem> newitem = std::make_shared<QM::QueueItem>(*item);
    // handle this in the AddItem
    newitem->id             = 0;
    newitem->created_at     = 0;
    newitem->updated_at     = 0;
    newitem->started_at     = 0;
    newitem->finished_at    = 0;
    newitem->status         = QM::QueueStatus::PENDING;
    newitem->status_message = "";
    newitem->stats          = QM::QueueItemStats();
    newitem->steps          = 0;
    newitem->step           = 0;
    newitem->time           = 0;

    // remove old images whitch are generated by the original job
    if (newitem->mode == QM::GenerationMode::TXT2IMG ||
        newitem->mode == QM::GenerationMode::IMG2IMG ||
        newitem->mode == QM::GenerationMode::UPSCALE) {
        newitem->images.erase(
            std::remove_if(newitem->images.begin(), newitem->images.end(),
                           [](QM::QueueItemImage img) {
                               return img.type == QM::QueueItemImageType::GENERATED;
                           }),
            newitem->images.end());
    }

    newitem->rawImages.clear();
    // re create images

    std::vector<QM::QueueItemImage*> tmpList;

    for (auto it = newitem->images.begin(); it != newitem->images.end(); it++) {
        QM::QueueItemImage* img = new QM::QueueItemImage(*it);
        tmpList.push_back(img);
    }
    newitem->images.clear();
    for (auto it = tmpList.begin(); it != tmpList.end(); it++) {
        newitem->images.push_back(*it);
    }
    tmpList.clear();

    // set to paused, user will start manually
    newitem->status = QM::QueueStatus::PAUSED;
    this->AddItem(newitem);
    return newitem;
}
int QM::QueueManager::AddItem(std::shared_ptr<QM::QueueItem> item, bool fromFile) {
    return this->AddItem(*item, fromFile);
}

std::shared_ptr<QM::QueueItem> QM::QueueManager::Duplicate(int id) {
    if (this->QueueList.find(id) != this->QueueList.end()) {
        return this->Duplicate(this->QueueList[id]);
    }
    return nullptr;
}

void QM::QueueManager::SetStatus(QM::QueueStatus status, std::shared_ptr<QM::QueueItem> item) {
    if (this->QueueList.find(item->id) != this->QueueList.end()) {
        if (item->finished_at == 0 && status == QM::QueueStatus::DONE) {
            item->finished_at = this->GetCurrentUnixTimestamp();
        } else {
            item->updated_at = this->GetCurrentUnixTimestamp();
        }
        this->QueueList[item->id]->status = status;
        this->SaveJobToFile(*this->QueueList[item->id]);
        this->SendEventToMainWindow(QM::QueueEvents::ITEM_STATUS_CHANGED, this->QueueList[item->id]);
    }
}

void QM::QueueManager::PauseAll() {
    for (auto q : this->QueueList) {
        if (q.second->status == QM::QueueStatus::PENDING) {
            this->SetStatus(QM::PAUSED, q.second);
        }
    }
}

void QM::QueueManager::RestartQueue() {
    for (auto q : this->QueueList) {
        if (q.second->status == QM::QueueStatus::PAUSED) {
            this->SetStatus(QM::PENDING, q.second);
        }
    }

    if (!this->isRunning) {
        for (auto job : this->QueueList) {
            if (job.second->status == QM::QueueStatus::PENDING) {
                if (this->isRunning == false) {
                    this->isRunning   = true;
                    this->currentItem = job.second;
                    this->SendEventToMainWindow(QM::QueueEvents::ITEM_START, job.second);
                }
                break;
            }
        }
    }
}

void QM::QueueManager::UnPauseItem(std::shared_ptr<QM::QueueItem> item) {
    if (item->status == QM::QueueStatus::PAUSED) {
        this->SetStatus(QM::QueueStatus::PENDING, item);
        // check if queue is active
        if (!this->isRunning) {
            this->currentItem = this->QueueList[item->id];
            this->SendEventToMainWindow(QM::QueueEvents::ITEM_START, this->QueueList[item->id]);
            this->isRunning = true;
        }
    }
}

void QM::QueueManager::PauseItem(std::shared_ptr<QM::QueueItem> item) {
    if (item->status == QM::QueueStatus::PENDING) {
        this->SetStatus(QM::QueueStatus::PAUSED, item);
    }
}

void QM::QueueManager::SendEventToMainWindow(QM::QueueEvents eventType, std::shared_ptr<QM::QueueItem> item) {
    // TODO: e->SetInt instead of SetString
    wxThreadEvent* e = new wxThreadEvent();
    e->SetString(wxString::Format("%d:%d", (int)sd_gui_utils::ThreadEvents::QUEUE, (int)eventType));
    e->SetPayload(item);
    wxQueueEvent(this->eventHandler, e);
}

void QM::QueueManager::OnThreadMessage(wxThreadEvent& e) {
    if (e.GetSkipped() == false) {
        e.Skip();
    }
    auto msg = e.GetString().ToStdString();

    std::string token                      = msg.substr(0, msg.find(":"));
    std::string content                    = msg.substr(msg.find(":") + 1);
    sd_gui_utils::ThreadEvents threadEvent = (sd_gui_utils::ThreadEvents)std::stoi(token);
    // only numbers here...

    // only handle the QUEUE messages, what this class generate
    if (threadEvent == sd_gui_utils::QUEUE) {
        QM::QueueEvents event = (QM::QueueEvents)std::stoi(content);
        auto payload          = e.GetPayload<std::shared_ptr<QM::QueueItem>>();
        if (event == QM::QueueEvents::ITEM_START) {
            this->SetStatus(QM::QueueStatus::RUNNING, payload);
            this->isRunning   = true;
            this->currentItem = payload;
            return;
        }
        if (event == QM::QueueEvents::ITEM_FINISHED) {
            this->SetStatus(QM::QueueStatus::DONE, payload);
            this->isRunning   = false;
            this->currentItem = nullptr;
            // jump to the next item in queue
            // find waiting jobs
            for (auto job : this->QueueList) {
                if (job.second->status == QM::QueueStatus::PENDING) {
                    if (this->isRunning == false) {
                        this->currentItem = job.second;
                        this->SendEventToMainWindow(QM::QueueEvents::ITEM_START, job.second);
                        this->isRunning = true;
                    }
                    break;
                }
            }
            return;
        }
        if (event == QM::QueueEvents::ITEM_MODEL_LOAD_START) {
            auto payload = e.GetPayload<std::shared_ptr<QM::QueueItem>>();
            this->SetStatus(QM::QueueStatus::MODEL_LOADING, payload);
        }
        if (event == QM::QueueEvents::ITEM_MODEL_FAILED) {
            auto payload = e.GetPayload<std::shared_ptr<QM::QueueItem>>();
            this->SetStatus(QM::QueueStatus::FAILED, payload);
            this->isRunning = false;
            // jump to the next
            for (auto job : this->QueueList) {
                if (job.second->status == QM::QueueStatus::PENDING) {
                    if (this->isRunning == false) {
                        this->currentItem = job.second;
                        this->SendEventToMainWindow(QM::QueueEvents::ITEM_START, job.second);
                        this->isRunning = true;
                    }
                    break;
                }
            }
        }
        if (event == QM::QueueEvents::ITEM_FAILED) {
            auto payload = e.GetPayload<std::shared_ptr<QM::QueueItem>>();
            this->SetStatus(QM::QueueStatus::FAILED, payload);
            this->isRunning = false;
            // jump to the next
            for (auto job : this->QueueList) {
                if (job.second->status == QM::QueueStatus::PENDING) {
                    if (this->isRunning == false) {
                        this->currentItem = job.second;
                        this->SendEventToMainWindow(QM::QueueEvents::ITEM_START, job.second);
                        this->isRunning = true;
                    }
                    break;
                }
            }
        }
        if (event == QM::QueueEvents::ITEM_GENERATION_STARTED) {
            auto payload = e.GetPayload<std::shared_ptr<QM::QueueItem>>();
            this->SetStatus(QM::QueueStatus::RUNNING, payload);
            this->isRunning = true;
        }
    }

    if (threadEvent == sd_gui_utils::ThreadEvents::HASHING_PROGRESS) {
        auto payload = e.GetPayload<std::shared_ptr<QM::QueueItem>>();
        this->SetStatus(QM::QueueStatus::HASHING, payload);
    }
}

void QM::QueueManager::SaveJobToFile(int id) {
    auto item = this->GetItemPtr(id);
    this->SaveJobToFile(*item);
}

void QM::QueueManager::SaveJobToFile(const QM::QueueItem& item) {
    try {
        nlohmann::json jsonfile(item);
        std::string filename = this->jobsDir + "/" + std::to_string(item.id) + ".json";
        std::ofstream file(filename);
        file << jsonfile;
        file.close();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}

bool QM::QueueManager::DeleteJob(const QM::QueueItem& item) {
    return this->DeleteJob(item.id);
}

bool QM::QueueManager::DeleteJob(int id) {
    auto item = this->GetItemPtr(id);
    if (item->id == 0) {
        return false;
    }
    std::string filename = this->jobsDir + "/" + std::to_string(item->id) + ".json";
    if (std::filesystem::exists(filename)) {
        if (std::filesystem::remove(filename)) {
            this->QueueList[item->id] = nullptr;
            this->QueueList.erase(item->id);
            return true;
        }
    }
    return false;
}

bool QM::QueueManager::IsRunning() {
    return this->isRunning;
}

int QM::QueueManager::GetCurrentUnixTimestamp(bool milliseconds) {
    const auto p1 = std::chrono::system_clock::now();
    auto duration = p1.time_since_epoch();

    if (milliseconds) {
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    } else {
        return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
    }
}

void QM::QueueManager::LoadJobListFromDir() {
    if (!std::filesystem::exists(this->jobsDir)) {
        std::filesystem::create_directories(this->jobsDir);
    }

    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(this->jobsDir)) {
        if (!dir_entry.exists() || !dir_entry.is_regular_file() || !dir_entry.path().has_extension()) {
            continue;
        }

        std::filesystem::path path = dir_entry.path();

        std::string ext = path.extension().string();

        if (ext != ".json") {
            continue;
        }

        std::string name = path.filename().replace_extension("").string();

        std::ifstream f(path.string());

        try {
            nlohmann::json data                 = nlohmann::json::parse(f);
            std::shared_ptr<QM::QueueItem> item = std::make_shared<QM::QueueItem>(data.get<QM::QueueItem>());
            if (item->status == QM::QueueStatus::RUNNING) {
                item->status = QM::QueueStatus::PAUSED;
            }
            if (item->status == QM::QueueStatus::MODEL_LOADING) {
                item->status = QM::QueueStatus::PAUSED;
            }
            this->AddItem(item, true);
        } catch (nlohmann::json::parse_error& ex) {
            std::cerr << "parse error at byte " << ex.byte << " in file: " << path.string() << std::endl;
        } catch (const nlohmann::json::out_of_range& e) {
            // output exception information
            std::cerr << "message: " << e.what() << '\n'
                      << "exception id: " << e.id << '\n'
                      << "in file: " << path.string() << std::endl;
        }
    }
}

int QM::QueueManager::GetAnId() {
    int id = this->GetCurrentUnixTimestamp(false);
    while (id <= this->lastId) {
        id++;
    }
    this->lastId = id;
    return id;
}

void QM::QueueManager::onItemAdded(QM::QueueItem item) {
    // this->parent->m_joblist
    // auto dataTable = this->parent->m_joblist;
}
