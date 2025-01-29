#include "TerminalApp.h"
#include "libs/SnowFlakeIdGenerarot.hpp"
#include "libs/subprocess.h"

bool TerminalApp::OnInit() {
    wxLog::SetTimestamp("%Y-%m-%d %H:%M:%S");

    if (argc < 2) {
        wxLogError("Usage: %s <config.json>", argv[0]);
        return false;
    }

    wxFileName config(argv[1]);
    if (config.FileExists() == false) {
        wxLogError("Config file not found: %s", argv[1]);
        return false;
    }

    wxFile file;
    wxString fileData;

    if (file.Open(config.GetAbsolutePath())) {
        file.ReadAll(&fileData);
        file.Close();
    } else {
        wxLogError("Failed to open config file: %s", config.GetAbsolutePath());
        return false;
    }

    try {
        nlohmann::json cfg = nlohmann::json::parse(fileData.utf8_string(), nullptr, true, true);
        auto cdata         = cfg.get<ServerConfig>();
        this->configData   = std::make_shared<ServerConfig>(cdata);
    } catch (const nlohmann::json::parse_error& e) {
        wxLogError("Error parsing config file: %s", e.what());
        return false;
    }

    wxInitAllImageHandlers();

    if (this->configData->data_path.empty()) {
        wxLogError("'data_path' is empty!");
        return false;
    }
    if (wxDirExists(this->configData->data_path) == false) {
        wxFileName ddir(this->configData->data_path);
        if (ddir.DirExists() == false) {
            ddir.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
            wxLogInfo("Created data dir: %s", ddir.GetFullPath());
        }
    }
    wxFileName ddirClients(this->configData->GetClientDataPath());
    if (ddirClients.DirExists() == false) {
        ddirClients.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        wxLogInfo("Created clients data dir: %s", ddirClients.GetFullPath());
    }
    // create jobs data dir
    wxFileName ddirJobs(this->configData->GetJobsPath());
    if (ddirJobs.DirExists() == false) {
        ddirJobs.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        wxLogInfo("Created jobs data dir: %s", ddirJobs.GetFullPath());
    }
    // set a random identifier to the server if not set in the config
    if (this->configData->authkey.empty()) {
        this->configData->authkey = sd_gui_utils::sha256_string_openssl(std::to_string(std::rand()) + this->configData->host + std::to_string(this->configData->port));
        wxLogInfo("Generated authkey: %s", this->configData->authkey);
        // save into the config file
        try {
            nlohmann::json saveJsonData = *this->configData.get();
            wxFile file;
            if (file.Open(config.GetAbsolutePath(), wxFile::write)) {
                file.Write(saveJsonData.dump(4));
                file.Close();
            } else {
                wxLogError("Failed to open config file: %s", config.GetAbsolutePath());
            }

        } catch (const nlohmann::json::parse_error& e) {
            wxLogError("Error parsing config file: %s", e.what());
            return false;
        }
    }
    if (this->configData->server_id.empty()) {
        auto now                    = std::chrono::system_clock::now();
        auto timestamp              = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        this->configData->server_id = sd_gui_utils::sha256_string_openssl(std::to_string(timestamp) + this->configData->authkey + this->configData->host + this->configData->model_paths.checkpoints);
        // auto idGen                  = sd_gui_utils::SnowflakeIDGenerator(this->configData->server_id, this->configData->host, this->configData->port);
        // this->configData->server_id = idGen.generateID(timestamp);

        wxLogInfo("Generated server_id: %s", this->configData->server_id);
        // save into the config file
        try {
            nlohmann::json saveJsonData = *this->configData.get();
            wxFile file;
            if (file.Open(config.GetAbsolutePath(), wxFile::write)) {
                file.Write(saveJsonData.dump(4));
                file.Close();
            } else {
                wxLogError("Failed to open config file: %s", config.GetAbsolutePath());
            }

        } catch (const nlohmann::json::parse_error& e) {
            wxLogError("Error parsing config file: %s", e.what());
            return false;
        }
    }

    if (configData->logfile.empty() == false) {
        configData->logfile = wxFileName(configData->logfile).GetAbsolutePath();
        wxLogInfo("Logging to file: %s", configData->logfile);

        this->logfile = std::fopen(configData->logfile.c_str(), "a+");
        if (this->logfile == nullptr) {
            wxLogError("Failed to open log file: %s", configData->logfile);
            return false;
        }
        this->logger = new wxLogStderr(this->logfile);
        wxASSERT(this->logger != nullptr);
        this->logger->SetFormatter(new CustomLogFormatter());
        this->oldLogger = wxLog::GetActiveTarget();
        wxLog::SetActiveTarget(this->logger);
        if (configData->diffuser_logfile.empty()) {
            wxFileName diffuser_logfile(configData->logfile);
            diffuser_logfile.SetExt("diffuser.log");
            configData->diffuser_logfile = diffuser_logfile.GetAbsolutePath();
            wxLogInfo("Diffuser log file: %s", configData->diffuser_logfile);
        }
    }

    this->queueManager       = std::make_shared<SimpleQueueManager>(this->configData->server_id, this->configData->GetJobsPath());
    this->snowflakeGenerator = std::make_shared<sd_gui_utils::SnowflakeIDGenerator>(this->configData->server_id);

    if (configData->shared_memory_path.empty()) {
        configData->shared_memory_path = std::string(SHARED_MEMORY_PATH) + this->configData->server_id;
    }

    if (this->configData->model_paths.checkpoints.empty()) {
        wxLogError("`model_paths->checkpoints` config key is missing");
        return false;
    }

    this->sharedMemoryManager = std::make_shared<SharedMemoryManager>(configData->shared_memory_path, SHARED_MEMORY_SIZE, true);
    wxLogInfo(wxString::Format("Shared memory initialized: %s size: %d", configData->shared_memory_path, SHARED_MEMORY_SIZE));

    Bind(wxEVT_TIMER, [this](wxTimerEvent& evt) {
        if (this->socket != nullptr) {
            this->socket->OnTimer();
        }
        evt.Skip();
    });

    this->timer.SetOwner(this, wxID_ANY);
    this->timer.Start(1000);

    this->eventHandlerReady.store(true);

    wxString currentPath = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();

    if (this->configData->exprocess_binary_path.empty() == false) {
        this->extprocessCommand = wxFileName(this->configData->exprocess_binary_path).GetAbsolutePath();
    } else {
        wxFileName f(currentPath, "");
        f.SetName(EPROCESS_BINARY_NAME);

        if (f.Exists() == false) {
            wxLogWarning("External process not found: %s", f.GetFullPath().utf8_string());
            f.AppendDir("extprocess");
        }
        if (f.Exists() == false) {
            wxLogWarning("External process not found: %s", f.GetFullPath().utf8_string());
            f.AppendDir("Debug");
        }

        if (f.Exists() == false) {
            wxLogError("External process not found: %s", f.GetFullPath().utf8_string());
            return false;
        }
        this->extprocessCommand = f.GetAbsolutePath();
    }
    if (wxFileName::Exists(this->extprocessCommand) == false) {
        wxLogError("External process not found: %s", this->extprocessCommand.utf8_string());
        return false;
    }
    std::string dllName = "stable-diffusion_";
    if (backend_type_to_str.contains(this->configData->backend) == false) {
        wxLogError("Unknown backend type: '%d'", this->configData->backend);
        return false;
    }
    dllName += backend_type_to_str.at(this->configData->backend);

    // get the library
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    wxString dllFullPath = currentPath + wxFileName::GetPathSeparator() + wxString::FromUTF8Unchecked(dllName.c_str());
    this->extProcessParams.Add(dllFullPath.utf8_string() + ".dll");

    if (wxFileExists(this->extProcessParams.begin()->c_str()) == false) {
        this->sendLogEvent("Shared lib not found: " + this->extProcessParams.begin()->c_str(), wxLOG_Error);
        return false;
    }
#else
    this->extProcessParams.Add("lib" + dllName + ".so");

#endif
    wxLogInfo("Using lib: %s", this->extProcessParams.begin()->c_str());
    this->sharedLibrary = std::make_shared<SharedLibrary>(this->extProcessParams.begin()->ToStdString());
    try {
        this->sharedLibrary->load();
    } catch (std::exception& e) {
        wxLogError("Error on loading shared library: '%s'", e.what());
        return false;
    }

    // load models
    if (this->LoadModelFiles() == false) {
        std::cerr << "Failed to load model files" << std::endl;
        return false;
    }
    // let the hashing begin
    this->CalcModelHashes();
    this->queueManager->LoadJobListFromDir();
    wxLogInfo("QueueManager loaded %" PRIu64 " jobs", this->queueManager->GetJobCount());

    this->extProcessParams.Add(this->configData->diffuser_logfile);
    this->extProcessParams.Add(this->configData->shared_memory_path);
    this->extProcessNeedToRun.store(true);

    this->command_line.push_back(this->extprocessCommand.c_str());

    this->params = this->extprocessCommand;
    for (auto const& p : this->extProcessParams) {
        this->params.Append(" ");
        this->params.Append(p);
        command_line.push_back(p.c_str().AsChar());
    }

    command_line.push_back(nullptr);

    return true;
    // return wxAppConsole::OnInit();  // Call the base class implementation
}

int TerminalApp::OnExit() {
    wxLogInfo("Exiting...");
    std::cout << "Exiting..." << std::endl;

    if (this->socket != nullptr) {
        this->socket->stop();
    }

    if (this->subprocess.load() != nullptr) {
        this->extProcessNeedToRun.store(false);
    }
    this->queueNeedToRun.store(false);

    for (auto& thread : this->threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    this->threads.clear();
    if (this->sharedLibrary != nullptr) {
        try {
            this->sharedLibrary->unload();
        } catch (std::exception& e) {
            wxLogError(e.what());
        }
    }
    if (this->logThread.joinable()) {
        this->logThread.join();
    }
    wxLog::SetActiveTarget(this->oldLogger);

    wxLogDebug("Stopping logger... bye!");
    delete this->logger;
    fclose(this->logfile);

    return wxAppConsole::OnExit();
}

int TerminalApp::OnRun() {
    this->logThread = std::thread(&TerminalApp::ProcessLogQueue, this);
    this->threads.emplace_back(&TerminalApp::ProcessOutputThread, this);
    this->threads.emplace_back(&TerminalApp::ExternalProcessRunner, this);
    this->threads.emplace_back(&TerminalApp::JobQueueThread, this);

    std::thread tr([this]() {
        this->sendLogEvent("Waiting for event handler thread to start", wxLOG_Info);
        while (this->eventHandlerReady.load() == false) {
            this->sendLogEvent("Waiting for event handler thread to start", wxLOG_Info);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        this->sendLogEvent("Waiting for external process to start", wxLOG_Info);
        while (this->extProcessNeedToRun.load() == true && this->extprocessIsRunning.load() == false) {
            this->sendLogEvent("Waiting for external process to start", wxLOG_Info);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        if (this->extProcessNeedToRun.load() == false && this->extprocessIsRunning.load() == false) {
            this->sendLogEvent("External process not running, exiting...", wxLOG_Error);
            this->ExitMainLoop();
            return;
        }
        this->sendLogEvent("Starting socket server", wxLOG_Info);
        try {
            // this->socket = new SocketApp(this->configData->host.c_str(), this->configData->port, this);
            this->socket = std::make_shared<SocketApp>(this->configData->host.c_str(), this->configData->port, this);
        } catch (std::exception& e) {
            this->sendLogEvent(wxString::Format("Socket server error: %s", e.what()), wxLOG_Error);
        }

        while (this->socket->isRunning() && this->eventHandlerReady.load() && this->extProcessNeedToRun.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        this->socket->stop();
        // stop the external process too
        this->extProcessNeedToRun.store(false);
        this->sendLogEvent("Socket server stopped", wxLOG_Info);
        this->ExitMainLoop();
    });

    this->threads.emplace_back(std::move(tr));

    return wxAppConsole::OnRun();
}

void TerminalApp::ProcessOutputThread() {
    this->sendLogEvent("Starting process output monitoring thread", wxLOG_Debug);
    while (this->extProcessNeedToRun.load() == true) {
        if (this->subprocess.load() != nullptr) {
            auto state = subprocess_alive(this->subprocess.load());
            if (state != 0) {
                static char stddata[1024]    = {0};
                static char stderrdata[1024] = {0};

                unsigned int size             = sizeof(stderrdata);
                unsigned int stdout_read_size = 0;
                unsigned int stderr_read_size = 0;

                stdout_read_size = subprocess_read_stdout(this->subprocess.load(), stddata, size);
                stderr_read_size = subprocess_read_stderr(this->subprocess.load(), stderrdata, size);

                // if (stdout_read_size > 0 && std::string(stddata).find("(null)") == std::string::npos) {
                //     this->sendLogEvent(stddata, wxLOG_Info);
                // }
                if (stderr_read_size > 0 && std::string(stderrdata).find("(null)") == std::string::npos) {
                    this->sendLogEvent(stddata, wxLOG_Error);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(EPROCESS_SLEEP_TIME));
    }
}
void TerminalApp::ExternalProcessRunner() {
    this->extprocessIsRunning.store(false);
    while (this->extProcessNeedToRun.load() == true) {
        if (this->subprocess.load() == nullptr) {
            auto sprocess = new subprocess_s();
            int result    = subprocess_create(command_line.data(), subprocess_option_no_window | subprocess_option_enable_async | subprocess_option_search_user_path | subprocess_option_inherit_environment, sprocess);
            this->subprocess.store(sprocess);
            if (result != 0 && this->extProcessNeedToRun.load() == true) {
                this->extprocessIsRunning.store(false);
                this->sendLogEvent("Failed to create subprocess, result: " + std::to_string(result) + " waiting 5s", wxLOG_Error);
                std::this_thread::sleep_for(std::chrono::seconds(5));
            } else {
                this->sendLogEvent("Subprocess started", wxLOG_Info);
                this->extprocessIsRunning.store(true);
            }
        }

        while (subprocess_alive(this->subprocess.load()) != 0 && this->extProcessNeedToRun.load() == true) {
            // std::unique_lock<std::mutex> lock(this->shmMutex);
            std::unique_ptr<char[]> buffer(new char[SHARED_MEMORY_SIZE]);

            this->sharedMemoryManager->read(buffer.get(), SHARED_MEMORY_SIZE);

            if (std::strlen(buffer.get()) > 0) {
                bool state = this->ProcessEventHandler(std::string(buffer.get(), std::strlen(buffer.get())));
                if (state == true) {
                    this->sharedMemoryManager->clear();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        if (this->extProcessNeedToRun.load() == true) {
            this->sendLogEvent("Subprocess stopped, need restarting", wxLOG_Error);
            // update the job if running
            if (this->queueManager != nullptr) {
                this->queueManager->ChangeCurrentJobStatus(QueueStatus::FAILED);
            }
            this->extprocessIsRunning.store(false);
            this->subprocess.store(nullptr);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    std::cout << "Subprocess thread stopped" << std::endl;
}
bool TerminalApp::ProcessEventHandler(std::string message) {
    if (message.empty()) {
        return false;
    }
    // this->itemUpdateEvent(message);
    try {
        nlohmann::json msg = nlohmann::json::parse(message);
        auto item          = msg.get<QueueItem>();
        if (item.status == QueueStatus::PENDING) {
            return false;
        }
        if (this->queueManager->UpdateCurrentJob(item, this->configData->GetJobsPath())) {
            auto updated_item = this->queueManager->GetItem(item.id);
            auto converted    = updated_item->convertToNetwork((updated_item->status == QueueStatus::DONE ? false : true), this->configData->server_id);
            sd_gui_utils::networks::Packet packet(sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE, sd_gui_utils::networks::Packet::Param::PARAM_JOB_UPDATE);
            packet.SetData(converted);
            this->socket->sendMsg(0, packet);  // send to everybody
            return true;
        }

    } catch (const std::exception& e) {
        this->sendLogEvent(wxString::Format("Extprocess parse error: %s", e.what()), wxLOG_Error);
        return true;
    }

    return false;
}

void TerminalApp::ProcessReceivedSocketPackages(sd_gui_utils::networks::Packet& packet) {
    if (packet.source_idx == -1) {
        this->sendLogEvent("Invalid source index", wxLOG_Error);
        return;
    }
    if (packet.param == sd_gui_utils::networks::Packet::Param::PARAM_JOB_DELETE) {
        sd_gui_utils::DeleteResponse response_data;
        response_data.job_id = packet.GetData<uint64_t>();
        response_data.state  = this->queueManager->DeleteItem(response_data.job_id, response_data.error);

        auto response = sd_gui_utils::networks::Packet(sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE, sd_gui_utils::networks::Packet::Param::PARAM_JOB_DELETE);
        response.SetData(response_data);
        this->socket->sendMsg(0, response);
        this->sendLogEvent("Sent job delete to client: " + std::to_string(packet.source_idx), wxLOG_Info);

        return;
    }
    if (packet.param == sd_gui_utils::networks::Packet::Param::PARAM_MODEL_LIST) {
        auto response = sd_gui_utils::networks::Packet(sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE, sd_gui_utils::networks::Packet::Param::PARAM_MODEL_LIST);
        std::vector<sd_gui_utils::networks::RemoteModelInfo> list;
        for (auto model : this->modelFiles) {
            // change the model's path
            model.second.path = this->configData->server_id + ":" + model.second.path;
            list.emplace_back(model.second);
        }
        response.SetData(list);
        this->socket->sendMsg(packet.source_idx, response);
        this->sendLogEvent("Sent model list to client: " + std::to_string(packet.source_idx), wxLOG_Info);
    }
    if (packet.param == sd_gui_utils::networks::Packet::Param::PARAM_JOB_IMAGE_LIST) {
        auto item_id = packet.GetData<uint64_t>();
        auto job     = this->queueManager->GetItem(item_id);

        if (job) {
            // update the entire job info
            auto response = sd_gui_utils::networks::Packet(sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE, sd_gui_utils::networks::Packet::Param::PARAM_JOB_UPDATE);
            response.SetData(job->convertToNetwork(false, this->configData->server_id));
            this->socket->sendMsg(packet.source_idx, response);
            this->sendLogEvent("Sent image list to client: " + std::to_string(packet.source_idx), wxLOG_Info);
        }
    }
    if (packet.param == sd_gui_utils::networks::Packet::Param::PARAM_JOB_LIST) {
        this->threads.emplace_back(std::thread([this, packet]() {
            this->sendLogEvent("Sending job list", wxLOG_Debug);
            auto list = this->queueManager->GetJobListCopy();
            for (auto& j : list) {
                auto response = sd_gui_utils::networks::Packet(
                    sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE,
                    sd_gui_utils::networks::Packet::Param::PARAM_JOB_UPDATE);
                response.SetData(j.convertToNetwork(true, this->configData->server_id));
                this->socket->sendMsg(packet.source_idx, response);
            }
            this->sendLogEvent("Sending job done", wxLOG_Debug);
            // this->sendLogEvent("Sent job list to client: " + std::to_string(packet.source_idx), wxLOG_Info);
        }));
    }
    if (packet.param == sd_gui_utils::networks::Packet::Param::PARAM_JOB_DUPLICATE) {
        auto job_id  = packet.GetData<uint64_t>();
        auto newItem = this->queueManager->DuplicateItem(job_id);
        if (newItem) {
            auto response = sd_gui_utils::networks::Packet(
                sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE,
                sd_gui_utils::networks::Packet::Param::PARAM_JOB_UPDATE);
            response.SetData(newItem->convertToNetwork(false, this->configData->server_id));
            this->socket->sendMsg(0, response);
        }
    }

    if (packet.param == sd_gui_utils::networks::Packet::Param::PARAM_JOB_PAUSE) {
        auto job_id = packet.GetData<uint64_t>();
        wxString error;
        auto newItem = this->queueManager->PauseItem(job_id, error);
        if (newItem) {
            auto response = sd_gui_utils::networks::Packet(
                sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE,
                sd_gui_utils::networks::Packet::Param::PARAM_JOB_UPDATE);
            response.SetData(newItem->convertToNetwork(true, this->configData->server_id));
            this->socket->sendMsg(0, response);
        } else {
            auto response = sd_gui_utils::networks::Packet(
                sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE,
                sd_gui_utils::networks::Packet::Param::PARAM_ERROR);
            response.SetData(error);
            this->socket->sendMsg(packet.source_idx, response);
        }
    }
    if (packet.param == sd_gui_utils::networks::Packet::Param::PARAM_JOB_RESUME) {
        auto job_id = packet.GetData<uint64_t>();
        wxString error;
        auto newItem = this->queueManager->ResumeItem(job_id, error);
        if (newItem) {
            auto response = sd_gui_utils::networks::Packet(
                sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE,
                sd_gui_utils::networks::Packet::Param::PARAM_JOB_UPDATE);
            response.SetData(newItem->convertToNetwork(true, this->configData->server_id));
            this->socket->sendMsg(0, response);
        } else {
            auto response = sd_gui_utils::networks::Packet(
                sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE,
                sd_gui_utils::networks::Packet::Param::PARAM_ERROR);
            response.SetData(error);
            this->socket->sendMsg(packet.source_idx, response);
        }
    }

    if (packet.param == sd_gui_utils::networks::Packet::Param::PARAM_JOB_ADD) {
        this->sendLogEvent("Received job add", wxLOG_Debug);
        auto data      = packet.GetData<sd_gui_utils::networks::RemoteQueueItem>();
        data.id        = this->queueManager->GenerateNextId();
        auto converted = QueueItem::convertFromNetwork(data, this->configData->GetJobsPath());
        converted.SetImagesPathsFromInfo();
        // set paths
        converted.params.embeddings_path = this->configData->model_paths.embedding;
        converted.params.lora_model_dir  = this->configData->model_paths.lora;
        converted.params.model_path      = this->GetModelPathByHash(converted.params.model_path);
        if (converted.params.model_path.empty()) {
            wxLogError("Model not found: %s", converted.model);
        }

        if (converted.params.vae_path.empty() == false) {
            const auto path = this->GetModelPathByHash(converted.params.vae_path);
            if (path.empty() == false) {
                converted.params.vae_path = path;
            } else {
                wxLogWarning("VAE Model not found: %s", converted.params.vae_path);
            }
        }
        if (converted.params.diffusion_model_path.empty() == false) {
            const auto path = this->GetModelPathByHash(converted.params.diffusion_model_path);
            if (path.empty() == false) {
                converted.params.diffusion_model_path = path;
            } else {
                wxLogWarning("Diffusion Model not found: %s", converted.params.diffusion_model_path);
            }
        }

        if (converted.params.taesd_path.empty() == false) {
            const auto path = this->GetModelPathByHash(converted.params.taesd_path);
            if (path.empty() == false) {
                converted.params.taesd_path = path;
            } else {
                wxLogWarning("TAESD Model not found: %s", converted.params.taesd_path);
            }
        }
        if (converted.params.esrgan_path.empty() == false) {
            const auto path = this->GetModelPathByHash(converted.params.esrgan_path);
            if (path.empty() == false) {
                converted.params.esrgan_path = path;
            } else {
                wxLogWarning("ESRGAN Model not found: %s", converted.params.esrgan_path);
            }
        }

        auto addedItem = this->queueManager->AddItem(converted);
        // send back the new item to all connected clients
        auto newConverted = addedItem->convertToNetwork(false, this->configData->server_id);
        sd_gui_utils::networks::Packet packet(sd_gui_utils::networks::Packet::Type::RESPONSE_TYPE, sd_gui_utils::networks::Packet::Param::PARAM_JOB_UPDATE);
        packet.SetData(newConverted);
        this->socket->sendMsg(0, packet);  // send to everybody
    }
}

bool TerminalApp::LoadModelFiles() {
    // this->sendLogEvent("Loading model files", wxLOG_Info);
    wxLogInfo("Loading model files");
    size_t used_sizes = 0;
    // load checkpoints
    if (this->configData->model_paths.checkpoints.empty()) {
        wxLogError("Model path is empty in config: %s", this->configData->model_paths.checkpoints);
        return false;
    }
    if (!wxDirExists(this->configData->model_paths.checkpoints)) {
        wxDir::Make(this->configData->model_paths.checkpoints, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }

    wxArrayString checkpoint_files;
    wxDir::GetAllFiles(this->configData->model_paths.checkpoints, &checkpoint_files);
    int checkpoint_count              = 0;
    wxULongLong used_checkpoint_sizes = 0;
    for (const auto& file : checkpoint_files) {
        wxFileName filename(file);
        if (std::find(CHECKPOINT_FILE_EXTENSIONS.begin(), CHECKPOINT_FILE_EXTENSIONS.end(), filename.GetExt()) != CHECKPOINT_FILE_EXTENSIONS.end()) {
            this->AddModelFile(filename, this->configData->model_paths.checkpoints, sd_gui_utils::DirTypes::CHECKPOINT, used_sizes, checkpoint_count, used_checkpoint_sizes);
        }
    }
    wxLogInfo("Loaded %d checkpoints %s", checkpoint_count, wxFileName::GetHumanReadableSize(used_checkpoint_sizes));

    // load loras
    if (this->configData->model_paths.lora.empty() == false && wxDirExists(this->configData->model_paths.lora)) {
        wxArrayString lora_files;
        wxDir::GetAllFiles(this->configData->model_paths.lora, &lora_files);
        int lora_count              = 0;
        wxULongLong used_lora_sizes = 0;
        for (const auto& file : lora_files) {
            wxFileName filename(file);
            if (std::find(LORA_FILE_EXTENSIONS.begin(), LORA_FILE_EXTENSIONS.end(), filename.GetExt()) != LORA_FILE_EXTENSIONS.end()) {
                this->AddModelFile(filename, this->configData->model_paths.lora, sd_gui_utils::DirTypes::LORA, used_sizes, lora_count, used_lora_sizes);
            }
        }
        wxLogInfo("Loaded %d loras %s", lora_count, wxFileName::GetHumanReadableSize(used_lora_sizes));
    } else {
        wxLogWarning("Lora path does not exist or missing from config: %s", this->configData->model_paths.lora);
    }

    // vae models
    if (this->configData->model_paths.vae.empty() == false && wxDirExists(this->configData->model_paths.vae)) {
        wxArrayString vae_files;
        wxDir::GetAllFiles(this->configData->model_paths.vae, &vae_files);
        int vae_count              = 0;
        wxULongLong used_vae_sizes = 0;
        for (const auto& file : vae_files) {
            wxFileName filename(file);
            if (std::find(VAE_FILE_EXTENSIONS.begin(), VAE_FILE_EXTENSIONS.end(), filename.GetExt()) != VAE_FILE_EXTENSIONS.end()) {
                this->AddModelFile(filename, this->configData->model_paths.vae, sd_gui_utils::DirTypes::VAE, used_sizes, vae_count, used_vae_sizes);
            }
        }
        wxLogInfo("Loaded %d vae models %s", vae_count, wxFileName::GetHumanReadableSize(used_vae_sizes));
    }
    // controlnet
    if (this->configData->model_paths.controlnet.empty() == false && wxDirExists(this->configData->model_paths.controlnet)) {
        wxArrayString controlnet_files;
        wxDir::GetAllFiles(this->configData->model_paths.controlnet, &controlnet_files);
        int controlnet_count              = 0;
        wxULongLong used_controlnet_sizes = 0;
        for (const auto& file : controlnet_files) {
            wxFileName filename(file);
            if (std::find(CONTROLNET_FILE_EXTENSIONS.begin(), CONTROLNET_FILE_EXTENSIONS.end(), filename.GetExt()) != CONTROLNET_FILE_EXTENSIONS.end()) {
                this->AddModelFile(filename, this->configData->model_paths.controlnet, sd_gui_utils::DirTypes::CONTROLNET, used_sizes, controlnet_count, used_controlnet_sizes);
            }
        }
        wxLogInfo("Loaded %d controlnet models %s", controlnet_count, wxFileName::GetHumanReadableSize(used_controlnet_sizes));
    }
    // esrgan models
    if (this->configData->model_paths.esrgan.empty() == false && wxDirExists(this->configData->model_paths.esrgan)) {
        wxArrayString esrgan_files;
        wxDir::GetAllFiles(this->configData->model_paths.esrgan, &esrgan_files);
        int esrgan_count              = 0;
        wxULongLong used_esrgan_sizes = 0;
        for (const auto& file : esrgan_files) {
            wxFileName filename(file);
            if (std::find(ESRGAN_FILE_EXTENSIONS.begin(), ESRGAN_FILE_EXTENSIONS.end(), filename.GetExt()) != ESRGAN_FILE_EXTENSIONS.end()) {
                this->AddModelFile(filename, this->configData->model_paths.esrgan, sd_gui_utils::DirTypes::ESRGAN, used_sizes, esrgan_count, used_esrgan_sizes);
            }
        }
        wxLogInfo("Loaded %d esrgan models %s", esrgan_count, wxFileName::GetHumanReadableSize(used_esrgan_sizes));
    }

    // load embeddings
    if (this->configData->model_paths.embedding.empty() == false && wxDirExists(this->configData->model_paths.embedding)) {
        wxArrayString embedding_files;
        wxDir::GetAllFiles(this->configData->model_paths.embedding, &embedding_files);
        int embedding_count              = 0;
        wxULongLong used_embedding_sizes = 0;
        for (const auto& file : embedding_files) {
            wxFileName filename(file);
            if (std::find(EMBEDDING_FILE_EXTENSIONS.begin(), EMBEDDING_FILE_EXTENSIONS.end(), filename.GetExt()) != EMBEDDING_FILE_EXTENSIONS.end()) {
                this->AddModelFile(filename, this->configData->model_paths.embedding, sd_gui_utils::DirTypes::EMBEDDING, used_sizes, embedding_count, used_embedding_sizes);
            }
        }
    }

    // load taesd
    if (this->configData->model_paths.taesd.empty() == false && wxDirExists(this->configData->model_paths.taesd)) {
        wxArrayString taesd_files;
        wxDir::GetAllFiles(this->configData->model_paths.taesd, &taesd_files);
        int taesd_count              = 0;
        wxULongLong used_taesd_sizes = 0;
        for (const auto& file : taesd_files) {
            wxFileName filename(file);
            if (std::find(TAESD_FILE_EXTENSIONS.begin(), TAESD_FILE_EXTENSIONS.end(), filename.GetExt()) != TAESD_FILE_EXTENSIONS.end()) {
                this->AddModelFile(filename, this->configData->model_paths.taesd, sd_gui_utils::DirTypes::TAESD, used_sizes, taesd_count, used_taesd_sizes);
            }
        }
    }

    // print stats
    wxLogInfo("Total size: %s", wxFileName::GetHumanReadableSize((wxULongLong)used_sizes));
    wxLogInfo("Need to hash: %s", wxFileName::GetHumanReadableSize(this->hashingFullSize.load()));
    return true;
}

void TerminalApp::ProcessLogQueue() {
    while (!this->eventHanlderExit.load()) {
        // std::lock_guard<std::mutex> lock(this->eventMutex);
        while (!this->eventQueue.IsEmpty()) {
            auto event = this->eventQueue.Pop();
            event();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
void TerminalApp::CalcModelHashes() {
    for (auto& model : this->modelFiles) {
        wxFileName hashFileName(model.second.remote_path);
        hashFileName.SetExt("sha256");
        if (wxFileExists(hashFileName.GetFullPath())) {
            wxFile f;
            if (f.Open(hashFileName.GetAbsolutePath(), wxFile::read)) {
                wxString hash;
                f.ReadAll(&hash);
                if (hash.IsEmpty() == false) {
                    model.second.sha256 = hash.SubString(0, 64).Trim().ToStdString();
                }
                f.Close();
                continue;
            }
            wxLogWarning("Failed to read hash file: %s", hashFileName.GetFullPath());
            continue;
        }

        // this->sendLogEvent(wxString::Format("Calculating hash for %s size: %s (%llu bytes)", model.second.name, model.second.size_f, model.second.size), wxLOG_Info);
        wxLogInfo("Calculating hash for %s size: %s (%llu bytes)", model.second.name, model.second.size_f, model.second.size);
        model.second.hash_fullsize = model.second.size;
        this->currentHashingItem   = std::make_shared<sd_gui_utils::RemoteModelInfo>(model.second);
        model.second.sha256        = sd_gui_utils::sha256_file_openssl(model.second.remote_path.c_str(), (void*)this, TerminalApp::FileHashCallBack);
        std::cout << std::endl;
        wxFile file;
        if (file.Open(hashFileName.GetFullPath(), wxFile::write)) {
            wxString fileContent = wxString::Format("%s\t%s", model.second.sha256, model.second.remote_path);
            file.Write(fileContent);
            file.Close();
            wxLogInfo("Hash calculated for %s %s", model.second.path, model.second.sha256);
            this->hashingProcessed.store(this->hashingProcessed.load() + static_cast<wxULongLong>(model.second.size));
        } else {
            wxLogError("Failed to calculate hash for %s", model.second.path);
            this->hashingFullSize.store(this->hashingFullSize.load() - static_cast<wxULongLong>(model.second.size));
        }
    }
}

void TerminalApp::AddModelFile(const wxFileName& filename, const std::string& rootpath, const sd_gui_utils::DirTypes type, size_t& used_sizes, int& file_count, wxULongLong& used_model_sizes) {
    sd_gui_utils::networks::RemoteModelInfo modelInfo(filename, type, rootpath);
    modelInfo.server_id                                        = this->configData->server_id;
    this->modelFiles[filename.GetAbsolutePath().utf8_string()] = modelInfo;
    used_sizes += modelInfo.size;
    used_model_sizes += modelInfo.size;
    file_count++;
    wxFileName hashFileName = filename;
    hashFileName.SetExt("sha256");
    if (!wxFileExists(hashFileName.GetFullPath())) {
        this->hashingFullSize.store(this->hashingFullSize.load() + static_cast<wxULongLong>(modelInfo.size));
    }
}