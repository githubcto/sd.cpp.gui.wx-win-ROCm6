#include "MainWindowUI.h"

MainWindowUI::MainWindowUI(wxWindow *parent)
    : UI(parent)
{
    this->ini_path = wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + "sd.ui.config.ini";
    this->sd_params = new sd_gui_utils::SDParams;
    this->currentInitialImage = new wxImage();
    this->currentInitialImagePreview = new wxImage();

    this->currentControlnetImage = new wxImage();
    this->currentControlnetImagePreview = new wxImage();

    this->ModelManager = new ModelInfo::Manager();

    // prepare data list views
    this->m_data_model_list->AppendTextColumn("Name", wxDATAVIEW_CELL_INERT, 200);
    this->m_data_model_list->AppendTextColumn("Size");
    this->m_data_model_list->AppendTextColumn("Type");
    this->m_data_model_list->AppendTextColumn("Hash");
    this->m_data_model_list->AppendProgressColumn("");

    this->m_joblist->AppendTextColumn("Id");
    this->m_joblist->AppendTextColumn("Created at");
    this->m_joblist->AppendTextColumn("Type");
    this->m_joblist->AppendTextColumn("Model");
    this->m_joblist->AppendTextColumn("Sampler");
    this->m_joblist->AppendTextColumn("Seed");
    this->m_joblist->AppendProgressColumn("Progress"); // progressbar
    this->m_joblist->AppendTextColumn("Speed");        // speed
    this->m_joblist->AppendTextColumn("Status");       // status

    this->m_joblist->GetColumn(0)->SetHidden(true);
    this->m_joblist->GetColumn(1)->SetSortable(true);
    this->m_joblist->GetColumn(1)->SetSortOrder(false);

    this->SetTitle(this->GetTitle() + SD_GUI_VERSION);
    this->TaskBar = new wxTaskBarIcon();

    // this->TaskBarMenu = new wxMenu();

    auto bitmap = app_png_to_wx_bitmap();
    TaskBar->SetIcon(bitmap, this->GetTitle());

    wxIcon icon;
    icon.CopyFromBitmap(bitmap);
    this->SetIcon(icon);

    this->cfg = new sd_gui_utils::config;
    this->fileConfig = new wxFileConfig("sd.cpp.ui", wxEmptyString, this->ini_path);
    wxConfigBase::Set(fileConfig);

    this->initConfig();

    wxPersistentRegisterAndRestore(this, this->GetName());

    this->qmanager = new QM::QueueManager(this->GetEventHandler(), this->cfg->jobs);

    // set SD logger
    sd_set_log_callback(MainWindowUI::HandleSDLog, (void *)this->GetEventHandler());

    // load
    this->LoadPresets();
    this->loadModelList();
    this->loadLoraList();
    this->loadVaeList();
    this->loadTaesdList();
    this->loadControlnetList();
    this->refreshModelTable();

    if (this->ModelFiles.size() > 0)
    {
        this->m_model->Enable();
    }
    if (this->VaeFiles.size() > 0)
    {
        this->m_vae->Enable();
    }
    if (this->TaesdFiles.size() > 0)
    {
        this->m_taesd->Enable();
    }
    if (this->ControlnetModels.size() > 0)
    {
        this->m_controlnetModels->Enable();
    }
    Bind(wxEVT_THREAD, &MainWindowUI::OnThreadMessage, this);
    const char *system_info = sd_get_system_info();
    this->logs->AppendText(system_info);
}

void MainWindowUI::onSettings(wxCommandEvent &event)
{
    this->settingsWindow = new MainWindowSettings(this);
    this->settingsWindow->Bind(wxEVT_CLOSE_WINDOW, &MainWindowUI::OnCloseSettings, this);
    this->settingsWindow->Show();
}

void MainWindowUI::onModelsRefresh(wxCommandEvent &event)
{
    this->loadModelList();
    this->loadVaeList();
    this->loadTaesdList();
    this->loadControlnetList();

    if (this->ModelFiles.size() > 0)
    {
        this->m_model->Enable();
    }
    else
    {
        this->m_model->Disable();
    }
    if (this->VaeFiles.size() > 0)
    {
        this->m_vae->Enable();
    }
    else
    {
        this->m_vae->Disable();
    }
    if (this->TaesdFiles.size() > 0)
    {
        this->m_taesd->Enable();
    }
    else
    {
        this->m_taesd->Disable();
    }
    if (this->ControlnetModels.size() > 0)
    {
        this->m_controlnetModels->Enable();
    }
    else
    {
        this->m_controlnetModels->Disable();
    }
}

void MainWindowUI::onModelSelect(wxCommandEvent &event)
{
    // check if really selected a model, or just the first element, which is always exists...
    auto name = this->m_model->GetStringSelection().ToStdString();
    auto first = this->m_model->GetString(0);
    if (name == first)
    {
        this->m_generate1->Disable();
        this->m_generate2->Disable();
        this->m_statusBar166->SetStatusText("Model: none");
        return;
    }
    this->m_generate1->Enable();
    this->m_generate2->Enable();
    this->sd_params->model_path = this->ModelFiles.at(name);
    this->m_statusBar166->SetStatusText("Model: " + this->sd_params->model_path);
}

void MainWindowUI::onTypeSelect(wxCommandEvent &event)
{
    wxChoice *c = (wxChoice *)event.GetEventObject();
    if (c->GetStringSelection() == "Q4_2 - not supported" || c->GetStringSelection() == "Q4_5 - not supported")
    {
        c->SetSelection(sizeof(sd_gui_utils::sd_type_names[0]) - 1);
    }
}

void MainWindowUI::onVaeSelect(wxCommandEvent &event)
{
    // TODO: Implement onVaeSelect
}

void MainWindowUI::onRandomGenerateButton(wxCommandEvent &event)
{
    this->m_seed->SetValue(sd_gui_utils::generateRandomInt(100000000, 999999999));
}

void MainWindowUI::onResolutionSwap(wxCommandEvent &event)
{
    auto oldW = this->m_width->GetValue();
    auto oldH = this->m_height->GetValue();

    this->m_height->SetValue(oldW);
    this->m_width->SetValue(oldH);
}

void MainWindowUI::onJobsStart(wxCommandEvent &event)
{
    // TODO: Implement onJobsStart
}

void MainWindowUI::onJobsPause(wxCommandEvent &event)
{
    // TODO: Implement onJobsPause
}

void MainWindowUI::onJobsDelete(wxCommandEvent &event)
{
    // TODO: Implement onJobsDelete
}

void MainWindowUI::OnJobListItemActivated(wxDataViewEvent &event)
{
    auto store = this->m_joblist->GetStore();

    auto row = this->m_joblist->GetSelectedRow();
    auto currentItem = store->GetItem(row);
    QM::QueueItem *qitem = reinterpret_cast<QM::QueueItem *>(store->GetItemData(currentItem));
    if (this->qmanager->GetItem(qitem->id).status == QM::QueueStatus::RUNNING ||
        this->qmanager->GetItem(qitem->id).status == QM::QueueStatus::PENDING ||
        this->qmanager->GetItem(qitem->id).status == QM::QueueStatus::HASHING)
    {
        return;
    }
    wxDisplay display(wxDisplay::GetFromWindow(this));
    wxRect screen = display.GetClientArea();
    MainWindowImageViewer *miv = new MainWindowImageViewer(this);
    miv->SetData(this->qmanager->GetItem(qitem->id));
    // miv->SetSize(screen.GetSize());
    // miv->SetPosition(wxPoint(0, 0));
    miv->Center();
    miv->SetThemeEnabled(true);
    miv->SetTitle(wxString::Format("Job details: %d", qitem->id));
    miv->Show();
}

void MainWindowUI::onContextMenu(wxDataViewEvent &event)
{

    auto column = event.GetColumn();
    if (column == -1)
    {
        return;
    }

    auto *source = (wxDataViewListCtrl *)event.GetEventObject();
    wxMenu *menu = new wxMenu();

    menu->SetClientData((void *)source);

    if (source == this->m_joblist)
    {
        auto item = event.GetItem();

        wxDataViewListStore *store = this->m_joblist->GetStore();
        QM::QueueItem *qitem = reinterpret_cast<QM::QueueItem *>(store->GetItemData(item));

        menu->Append(1, "Requeue");

        menu->Append(2, wxString::Format("Copy to text2img %d", qitem->id));
        menu->Append(3, wxString::Format("Copy prompts to text2img %d", qitem->id));
        menu->Append(4, wxString::Format("Copy prompts to img2img %d", qitem->id));
        menu->Append(5, "Details...");
        menu->AppendSeparator();
        menu->Append(99, "Delete");
        if (this->qmanager->GetItem(qitem->id).status == QM::QueueStatus::RUNNING || this->qmanager->GetItem(qitem->id).status == QM::QueueStatus::HASHING)
        {
            menu->Enable(1, false);
            menu->Enable(5, false);
            menu->Enable(99, false);
        }
    }

    if (source == this->m_data_model_list)
    {
        // auto item = event.GetItem();
        auto item = this->m_data_model_list->GetCurrentItem();

        wxDataViewListStore *store = this->m_data_model_list->GetStore();
        sd_gui_utils::ModelFileInfo *modelinfo = reinterpret_cast<sd_gui_utils::ModelFileInfo *>(store->GetItemData(item));

        menu->Append(111, "Download info from CivitAi.com - not implemented yet");
        menu->Enable(111, false);

        if (!modelinfo->sha256.empty())
        {
            menu->Append(100, "RE-Calculate Hash");
        }
        else
        {
            menu->Append(100, "Calculate Hash");
        }

        if (modelinfo->model_type == sd_gui_utils::DirTypes::CHECKPOINT)
        {
            menu->Append(200, wxString::Format("Select model %s to the next job", modelinfo->name));
        }
        if (modelinfo->model_type == sd_gui_utils::DirTypes::LORA)
        {
            menu->Append(101, wxString::Format("Append to text2img prompt <lora:%s:0.5>", modelinfo->name));
            menu->Append(102, wxString::Format("Append to text2img neg. prompt <lora:%s:0.5>", modelinfo->name));
            menu->Append(103, wxString::Format("Append to img2img prompt <lora:%s:0.5>", modelinfo->name));
            menu->Append(104, wxString::Format("Append to img2img neg. prompt <lora:%s:0.5>", modelinfo->name));
        }
    }
    menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainWindowUI::OnPopupClick, this);
    PopupMenu(menu);
}

void MainWindowUI::onGenerate(wxCommandEvent &event)
{
    this->initConfig();

    // get the generation type
    // 0 -> joblist
    // 1 -> text2img
    // 3 -> img2img
    // 4 -> models
    // 5 -> ... none
    // ...
    auto type = QM::GenerationMode::TXT2IMG;
    int pageId = this->m_notebook1302->GetSelection();
    if (pageId == 1)
    {
        type = QM::GenerationMode::TXT2IMG;
    }
    if (pageId == 2)
    {
        type = QM::GenerationMode::IMG2IMG;
    }

    // prepare params
    this->sd_params->model_path = this->ModelFiles.at(this->m_model->GetStringSelection().ToStdString());

    if (this->m_taesd->GetCurrentSelection() > 0)
    {
        this->sd_params->taesd_path = this->TaesdFiles.at(this->m_taesd->GetStringSelection().ToStdString());
    }
    else
    {
        this->sd_params->taesd_path = "";
    }
    if (this->m_vae->GetCurrentSelection() > 0)
    {
        this->sd_params->vae_path = this->VaeFiles.at(this->m_vae->GetStringSelection().ToStdString());
    }
    else
    {
        this->sd_params->vae_path = "";
    }

    this->sd_params->lora_model_dir = this->cfg->lora;
    this->sd_params->embeddings_path = this->cfg->embedding;
    this->sd_params->n_threads = this->cfg->n_threads;

    if (type == QM::GenerationMode::TXT2IMG)
    {
        this->sd_params->prompt = this->m_prompt->GetValue().ToStdString();
        this->sd_params->negative_prompt = this->m_neg_prompt->GetValue().ToStdString();
    }
    if (type == QM::GenerationMode::IMG2IMG)
    {
        this->sd_params->prompt = this->m_prompt2->GetValue().ToStdString();
        this->sd_params->negative_prompt = this->m_neg_prompt2->GetValue().ToStdString();
    }
    this->sd_params->cfg_scale = static_cast<float>(this->m_cfg->GetValue());
    this->sd_params->seed = this->m_seed->GetValue();
    this->sd_params->clip_skip = this->m_clip_skip->GetValue();
    this->sd_params->sample_steps = this->m_steps->GetValue();

    if (type == QM::GenerationMode::TXT2IMG)
    {
        this->sd_params->control_strength = this->m_controlnetStrength->GetValue();
    }

    if (type == QM::GenerationMode::IMG2IMG)
    {
        this->sd_params->strength = this->m_strength->GetValue();
    }

    if (this->m_controlnetModels->GetCurrentSelection() > 0 && type == QM::GenerationMode::TXT2IMG)
    {
        this->sd_params->controlnet_path = this->ControlnetModels.at(this->m_controlnetModels->GetStringSelection().ToStdString());
        // do not set the control image if we have no model
        if (this->m_controlnetImageOpen->GetPath().length() > 0)
        {
            this->sd_params->control_image_path = this->m_controlnetImageOpen->GetPath().ToStdString();
        }
    }
    else
    {
        this->sd_params->controlnet_path = "";
        this->sd_params->control_image_path = "";
    }

    this->sd_params->sample_method = (sample_method_t)this->m_sampler->GetCurrentSelection();

    if (this->m_type->GetCurrentSelection() != 0)
    {
        auto selected = this->m_type->GetStringSelection();
        for (auto types : sd_gui_utils::sd_type_gui_names)
        {
            if (types.second == selected)
            {
                this->sd_params->wtype = (sd_type_t)types.first;
            }
        }
    }
    else
    {
        this->sd_params->wtype = sd_type_t::SD_TYPE_COUNT;
    }

    this->sd_params->batch_count = this->m_batch_count->GetValue();

    this->sd_params->width = this->m_width->GetValue();
    this->sd_params->height = this->m_height->GetValue();

    // this->sd_params->vae_tiling
    this->sd_params->vae_tiling = this->m_vae_tiling->GetValue();
    // at img2img is false
    // this->sd_params->vae_decode_only = this->m_vae_decode_only->GetValue();

    QM::QueueItem item;
    item.params = *this->sd_params;
    item.model = this->m_model->GetStringSelection().ToStdString();
    item.mode = type;
    if (type == QM::GenerationMode::IMG2IMG)
    {
        item.initial_image = this->currentInitialImagePath;
    }

    if (item.params.seed == -1)
    {
        item.params.seed = sd_gui_utils::generateRandomInt(100000000, 999999999);
        this->m_seed->SetValue(item.params.seed);
    }

    // add the queue item
    auto id = this->qmanager->AddItem(item);
}

void MainWindowUI::OnControlnetImageOpen(wxFileDirPickerEvent &event)
{
    if (event.GetPath().empty())
    {
        return;
    }
    wxImage img;
    img.LoadFile(event.GetPath());

    auto preview = sd_gui_utils::ResizeImageToMaxSize(img, 200, 190);

    this->currentControlnetImage = new wxImage(img);
    this->currentControlnetImagePreview = new wxImage(preview);

    this->m_controlnetImagePreview->SetBitmap(preview);
    this->m_controlnetImagePreview->Show();
    this->m_controlnetImagePreviewButton->Enable();
    this->m_controlnetImageDelete->Enable();
    this->m_width->SetValue(img.GetWidth());
    this->m_height->SetValue(img.GetHeight());
    // can not change the outpt images resolution
    this->m_width->Disable();
    this->m_height->Disable();
    this->m_button7->Disable(); // swap resolution
    this->m_controlnetImagePreview->SetSize(200, 190);
}

void MainWindowUI::OnControlnetImagePreviewButton(wxCommandEvent &event)
{
    MainWindowImageDialog *dialog = new MainWindowImageDialog(this);
    wxImage img(*this->currentControlnetImage);
    auto size = img.GetSize();

    dialog->SetSize(size.GetWidth() + 100, size.GetHeight() + 100);
    wxString title = wxString::Format("Controlnet Image %dx%dpx", size.GetWidth(), size.GetHeight());
    dialog->SetTitle(title);
    dialog->m_bitmap->SetBitmap(img);
    dialog->ShowModal();
}

void MainWindowUI::OnControlnetImageDelete(wxCommandEvent &event)
{
    this->m_controlnetImagePreviewButton->Disable();
    this->m_controlnetImageDelete->Disable();
    this->m_controlnetImageOpen->SetPath("");
    this->currentControlnetImage = NULL;
    this->currentControlnetImagePreview = NULL;
    this->m_controlnetImagePreview->SetSize(200, 190);
    this->m_controlnetImagePreview->SetBitmap(wxBitmap());
    this->m_width->Enable();
    this->m_height->Enable();
    this->m_button7->Enable(); // swap resolution
    this->m_controlnetModels->Select(0);
}

void MainWindowUI::Onimg2imgDropFile(wxDropFilesEvent &event)
{
    // only just one file.. sry...
    auto files = event.GetFiles();
    auto file = files[0];

    wxImage img;
    if (img.LoadFile(file))
    {

        auto preview = sd_gui_utils::ResizeImageToMaxSize(img, 300, 210);

        this->currentInitialImage = new wxImage(img);
        this->currentInitialImagePreview = new wxImage(preview);
        this->currentInitialImagePath = file;

        this->m_img2img_preview->SetBitmap(preview);
        this->m_img2im_preview_img->Enable();
        this->m_delete_initial_img->Enable();
        this->m_generate1->Enable();
        wxString comment = img.GetOption(wxT("COM"));

        if (!comment.IsEmpty())
        {
            wxMessageBox(wxString::Format("Found some meta info: \n%s", comment));
        }
    }
    else
    {
        wxMessageBox("Can not open image!");
    }
}

void MainWindowUI::OnImageOpenFileChanged(wxFileDirPickerEvent &event)
{

    if (event.GetPath().empty())
    {
        this->m_generate1->Disable();
        return;
    }

    wxImage img;
    if (img.LoadFile(event.GetPath()))
    {

        auto preview = sd_gui_utils::ResizeImageToMaxSize(img, 300, 210);

        this->currentInitialImage = new wxImage(img);
        this->currentInitialImagePreview = new wxImage(preview);
        this->currentInitialImagePath = event.GetPath();

        this->m_img2img_preview->SetBitmap(preview);
        this->m_img2im_preview_img->Enable();
        this->m_delete_initial_img->Enable();
        this->m_generate1->Enable();

        wxString comment = img.GetOption(wxT("COM"));

        if (!comment.IsEmpty())
        {
            wxMessageBox(wxString::Format("Found some meta info: \n%s", comment));
        }
    }
    else
    {
        wxMessageBox("Can not open image!");
    }
}

void MainWindowUI::OnImg2ImgPreviewButton(wxCommandEvent &event)
{
    MainWindowImageDialog *dialog = new MainWindowImageDialog(this);
    wxImage img(*this->currentInitialImage);
    auto size = img.GetSize();

    dialog->SetSize(size.GetWidth() + 100, size.GetHeight() + 100);
    dialog->SetTitle("IMG2IMG - original image");
    dialog->m_bitmap->SetBitmap(img);
    dialog->ShowModal();
}

void MainWindowUI::OnDeleteInitialImage(wxCommandEvent &event)
{
    this->currentInitialImage = NULL;
    this->currentInitialImagePreview = NULL;
    this->currentInitialImagePath = "";
    this->m_img2img_preview->SetBitmap(wxBitmap());
    this->m_img2im_preview_img->Disable();
    this->m_delete_initial_img->Disable();
}

void MainWindowUI::OnCheckboxLoraFilter(wxCommandEvent &event)
{
    std::bitset<10> t;

    if (this->m_checkbox_filter_checkpoints->GetValue() && this->m_checkbox_lora_filter->GetValue())
    {
        t.set(sd_gui_utils::LORA);
        t.set(sd_gui_utils::CHECKPOINT);
    }

    if (this->m_checkbox_filter_checkpoints->GetValue() == false)
    {
        t.reset();
        t.set(sd_gui_utils::LORA);
    }

    if (this->m_checkbox_lora_filter->GetValue() == false)
    {
        t.reset();
        t.set(sd_gui_utils::CHECKPOINT);
    }
    auto value = this->m_modellist_filter->GetValue().ToStdString();

    if (value.length() == 0)
    {
        this->refreshModelTable(value, t);
    }
    else
    {
        this->refreshModelTable("", t);
    }
}

void MainWindowUI::OnCheckboxCheckpointFilter(wxCommandEvent &event)
{
    std::bitset<10> t;

    if (this->m_checkbox_filter_checkpoints->GetValue() && this->m_checkbox_lora_filter->GetValue())
    {
        t.set(sd_gui_utils::LORA);
        t.set(sd_gui_utils::CHECKPOINT);
    }

    if (this->m_checkbox_filter_checkpoints->GetValue() == false)
    {
        t.reset();
        t.set(sd_gui_utils::LORA);
    }

    if (this->m_checkbox_lora_filter->GetValue() == false)
    {
        t.reset();
        t.set(sd_gui_utils::CHECKPOINT);
    }
    auto value = this->m_modellist_filter->GetValue().ToStdString();

    if (value.length() == 0)
    {
        this->refreshModelTable(value, t);
    }
    else
    {
        this->refreshModelTable("", t);
    }
}

void MainWindowUI::OnModellistFilterKeyUp(wxKeyEvent &event)
{
    std::bitset<10> t;

    if (this->m_checkbox_filter_checkpoints->GetValue() && this->m_checkbox_lora_filter->GetValue())
    {
        t.set(sd_gui_utils::LORA);
        t.set(sd_gui_utils::CHECKPOINT);
    }

    if (this->m_checkbox_filter_checkpoints->GetValue() == false)
    {
        t.reset();
        t.set(sd_gui_utils::LORA);
    }

    if (this->m_checkbox_lora_filter->GetValue() == false)
    {
        t.reset();
        t.set(sd_gui_utils::CHECKPOINT);
    }

    auto value = this->m_modellist_filter->GetValue().ToStdString();

    if (value.length() == 0)
    {
        this->refreshModelTable(value, t);
    }
    if (value.length() <= 3)
    {
        return;
    }
    this->m_data_model_list->DeleteAllItems();
    this->refreshModelTable(value, t);
}

void MainWindowUI::OnDataModelActivated(wxDataViewEvent &event)
{
    auto store = this->m_data_model_list->GetStore();
    auto row = this->m_data_model_list->GetSelectedRow();
    auto currentItem = store->GetItem(row);
    sd_gui_utils::ModelFileInfo *_item = reinterpret_cast<sd_gui_utils::ModelFileInfo *>(store->GetItemData(currentItem));
    auto model = this->ModelManager->getInfo(_item->path);
    if (model.sha256.empty())
    {
        wxMessageBox("Please check hash first!");
        return;
    }
    MainWindowModelinfo *mi = new MainWindowModelinfo(this);

    if (mi->SetData(this->ModelManager, _item->path, this->cfg->datapath))
    {
        mi->Show();
    }
    else
    {
        mi = NULL;
        delete mi;
    }
}

void MainWindowUI::onSamplerSelect(wxCommandEvent &event)
{
    this->sd_params->sample_method = (sample_method_t)this->m_sampler->GetSelection();
}

void MainWindowUI::onSavePreset(wxCommandEvent &event)
{
    wxTextEntryDialog dlg(this, "Please specify a name (only alphanumeric)");
    dlg.SetTextValidator(wxFILTER_ALPHA | wxFILTER_DIGITS);
    if (dlg.ShowModal() == wxID_OK)
    {
        sd_gui_utils::generator_preset preset;

        wxString preset_name = dlg.GetValue();
        preset.cfg = this->m_cfg->GetValue();
        // do not save the seed
        // preset.seed = this->m_seed->GetValue();
        preset.clip_skip = this->m_clip_skip->GetValue();
        preset.steps = this->m_steps->GetValue();
        preset.width = this->m_width->GetValue();
        preset.height = this->m_height->GetValue();
        preset.sampler = (sample_method_t)this->m_sampler->GetSelection();
        preset.batch = this->m_batch_count->GetValue();
        preset.name = preset_name.ToStdString();
        preset.mode = "text2image";
        nlohmann::json j(preset);
        std::string presetfile = fmt::format("{}{}{}.json",
                                             this->cfg->presets,
                                             wxString(wxFileName::GetPathSeparator()).ToStdString(),
                                             preset.name);

        std::ofstream file(presetfile);
        file << j;
        file.close();
        this->LoadPresets();
    }
}

void MainWindowUI::onLoadPreset(wxCommandEvent &event)
{
    auto selected = this->m_preset_list->GetCurrentSelection();
    auto name = this->m_preset_list->GetString(selected);

    for (auto preset : this->Presets)
    {
        if (preset.second.name == name)
        {
            this->m_cfg->SetValue(preset.second.cfg);
            this->m_clip_skip->SetValue(preset.second.clip_skip);
            // do not save seed
            // this->m_seed->SetValue(preset.second.seed);
            this->m_steps->SetValue(preset.second.steps);
            // when the width || height input is disabled, do not modify it (eg.: controlnet works...)
            if (this->m_width->IsEnabled() || this->m_height->IsEnabled())
            {
                this->m_width->SetValue(preset.second.width);
                this->m_height->SetValue(preset.second.height);
            }
            this->m_sampler->SetSelection(preset.second.sampler);
            this->m_batch_count->SetValue(preset.second.batch);
        }
    }
}

void MainWindowUI::onSelectPreset(wxCommandEvent &event)
{
    if (this->m_preset_list->GetCurrentSelection() == 0)
    {
        this->m_load_preset->Disable();
        this->m_delete_preset->Disable();
    }
    else
    {
        this->m_load_preset->Enable();
        this->m_delete_preset->Enable();
    }
}

void MainWindowUI::onDeletePreset(wxCommandEvent &event)
{

    auto name = this->m_preset_list->GetStringSelection().ToStdString();

    if (this->Presets.find(name) != this->Presets.end())
    {
        auto preset = this->Presets[name];
        std::remove(preset.path.c_str());
        this->LoadPresets();
    }
}

void MainWindowUI::ModelHashingCallback(size_t readed_size, std::string sha256, void *custom_pointer)
{
    sd_gui_utils::VoidHolder *holder = static_cast<sd_gui_utils::VoidHolder *>(custom_pointer);
    wxEvtHandler *eventHandler = (wxEvtHandler *)holder->p1;
    QM::QueueItem *myItem = (QM::QueueItem *)holder->p2;
    myItem->hash_progress_size = readed_size;
    wxThreadEvent *e = new wxThreadEvent();
    e->SetString("HASHING_PROGRESS:placeholder");
    e->SetPayload(*myItem);
    wxQueueEvent(eventHandler, e);
}

void MainWindowUI::OnThreadMessage(wxThreadEvent &e)
{
    if (e.GetSkipped() == false)
    {
        e.Skip();
    }
    auto msg = e.GetString().ToStdString();

    std::string token = msg.substr(0, msg.find(":"));
    std::string content = msg.substr(msg.find(":") + 1);

    // this->logs->AppendText(fmt::format("Got thread message: {}\n", e.GetString().ToStdString()));
    if (token == "QUEUE")
    {
        // only numbers here...
        QM::QueueEvents event = (QM::QueueEvents)std::stoi(content);
        // only handle the QUEUE messages, what this class generate
        // alway QM::EueueItem the payload, with the new data
        QM::QueueItem payload;
        payload = e.GetPayload<QM::QueueItem>();
        switch (event)
        {
            // new item added
        case QM::QueueEvents::ITEM_ADDED:
            this->OnQueueItemManagerItemAdded(payload);
            break;
            // item status changed
        case QM::QueueEvents::ITEM_STATUS_CHANGED:
            this->OnQueueItemManagerItemStatusChanged(payload);
            break;
            // item updated... ? ? ?
        case QM::QueueEvents::ITEM_UPDATED:
            this->OnQueueItemManagerItemUpdated(payload);
            break;
        case QM::QueueEvents::ITEM_START:
            this->StartGeneration(payload);
            break;

        default:
            break;
        }
    }
    if (token == "MODEL_LOAD_DONE")
    {

        this->modelLoaded = true;
    }
    if (token == "MODEL_LOAD_START")
    {
        this->logs->AppendText(fmt::format("Model load start: {}\n", content));
    }
    if (token == "MODEL_LOAD_ERROR")
    {

        this->logs->AppendText(fmt::format("Model load error: {}\n", content));
        this->modelLoaded = false;
    }

    if (token == "GENERATION_START")
    {
        auto myjob = e.GetPayload<QM::QueueItem>();
        this->logs->AppendText(fmt::format("Diffusion started. Seed: {} Batch: {} {}x{}px Cfg: {} Steps: {}\n",
                                           myjob.params.seed,
                                           myjob.params.batch_count,
                                           myjob.params.width,
                                           myjob.params.height,
                                           myjob.params.cfg_scale,
                                           myjob.params.sample_steps));
    }
    /// status of hashing from modellist
    if (token == "STANDALONE_HASHING_PROGRESS")
    {
        // modelinfo
        sd_gui_utils::ModelFileInfo *modelinfo = e.GetPayload<sd_gui_utils::ModelFileInfo *>();
        auto store = this->m_data_model_list->GetStore();
        int progressCol = this->m_data_model_list->GetColumnCount() - 1; // progressbar col
        int hashCol = this->m_data_model_list->GetColumnCount() - 2;
        size_t _x = modelinfo->hash_progress_size;
        size_t _m = modelinfo->hash_fullsize;
        int current_progress = 0;
        auto _hr1 = sd_gui_utils::humanReadableFileSize(static_cast<double>(_x));
        auto _hr2 = sd_gui_utils::humanReadableFileSize(static_cast<double>(_m));

        if (_m != 0)
        {
            current_progress = static_cast<int>((_x * 100) / _m);
        }

        this->m_statusBar166->SetStatusText(fmt::format("Hashing: {} / {}  {}% {}",
                                                        wxString::Format("%.1f %s", _hr1.first, _hr1.second).ToStdString(),
                                                        wxString::Format("%.1f %s", _hr2.first, _hr2.second).ToStdString(),
                                                        current_progress,
                                                        modelinfo->name));

        for (unsigned int i = 0; i < store->GetItemCount(); i++)
        {
            auto _item = store->GetItem(i);
            auto _item_data = store->GetItemData(_item);
            auto *_qitem = reinterpret_cast<sd_gui_utils::ModelFileInfo *>(_item_data);
            if (_qitem->name == modelinfo->name)
            {
                store->SetValueByRow(current_progress, i, progressCol);
                store->SetValueByRow(modelinfo->sha256, i, hashCol);
                this->m_data_model_list->Refresh();
                break;
            }
        }
    }
    if (token == "STANDALONE_HASHING_DONE")
    {
        this->m_statusBar166->SetStatusText("");
        sd_gui_utils::ModelFileInfo *modelinfo = e.GetPayload<sd_gui_utils::ModelFileInfo *>();
        nlohmann::json j(*modelinfo);
        std::ofstream file(modelinfo->meta_file);
        file << j;
        file.close();
    }
    if (token == "HASHING_PROGRESS")
    {
        QM::QueueItem myjob = e.GetPayload<QM::QueueItem>();

        // update column
        auto store = this->m_joblist->GetStore();

        int progressCol = this->m_joblist->GetColumnCount() - 3;

        size_t _x = myjob.hash_progress_size;
        size_t _m = myjob.hash_fullsize;
        int current_progress = 0;
        auto _hr1 = sd_gui_utils::humanReadableFileSize(static_cast<double>(_x));
        auto _hr2 = sd_gui_utils::humanReadableFileSize(static_cast<double>(_m));

        if (_m != 0)
        {
            current_progress = static_cast<int>((_x * 100) / _m);
        }

        this->m_statusBar166->SetStatusText(fmt::format("Hashing: {} / {}  {}% {}",
                                                        wxString::Format("%.1f %s", _hr1.first, _hr1.second).ToStdString(),
                                                        wxString::Format("%.1f %s", _hr2.first, _hr2.second).ToStdString(),
                                                        current_progress,
                                                        myjob.model));

        for (unsigned int i = 0; i < store->GetItemCount(); i++)
        {
            auto _item = store->GetItem(i);
            auto _item_data = store->GetItemData(_item);
            auto *_qitem = reinterpret_cast<QM::QueueItem *>(_item_data);
            if (_qitem->id == myjob.id)
            {
                store->SetValueByRow(current_progress, i, progressCol);
                this->m_joblist->Refresh();
                break;
            }
        }
    }
    if (token == "HASHING_DONE")
    {
        QM::QueueItem myjob = e.GetPayload<QM::QueueItem>();

        // update column
        auto store = this->m_joblist->GetStore();

        int progressCol = this->m_joblist->GetColumnCount() - 3;

        for (unsigned int i = 0; i < store->GetItemCount(); i++)
        {
            auto _item = store->GetItem(i);
            auto _item_data = store->GetItemData(_item);
            auto *_qitem = reinterpret_cast<QM::QueueItem *>(_item_data);
            if (_qitem->id == myjob.id)
            {
                store->SetValueByRow(0, i, progressCol);
                this->m_joblist->Refresh();
                break;
            }
        }
    }
    // in the original SD.cpp the progress callback is not implemented... :(
    if (token == "GENERATION_PROGRESS")
    {
        QM::QueueItem myjob = e.GetPayload<QM::QueueItem>();

        // update column
        auto store = this->m_joblist->GetStore();
        wxString speed = wxString::Format(myjob.time > 1.0f ? "%.2fs/it" : "%.2fit/s", myjob.time > 1.0f || myjob.time == 0 ? myjob.time : (1.0f / myjob.time));
        int progressCol = this->m_joblist->GetColumnCount() - 3;
        int speedCol = this->m_joblist->GetColumnCount() - 2;
        float current_progress = 100.f * (static_cast<float>(myjob.step) / static_cast<float>(myjob.steps));

        for (unsigned int i = 0; i < store->GetItemCount(); i++)
        {
            auto _item = store->GetItem(i);
            auto _item_data = store->GetItemData(_item);
            auto *_qitem = reinterpret_cast<QM::QueueItem *>(_item_data);
            if (_qitem->id == myjob.id)
            {
                store->SetValueByRow(static_cast<int>(current_progress), i, progressCol);
                store->SetValueByRow(speed, i, speedCol);
                this->m_joblist->Refresh();
                break;
            }
        }
    }
    if (token == "GENERATION_ERROR")
    {
        this->logs->AppendText(fmt::format("Generation error: {}\n", content));
    }
    if (token == "SD_MESSAGE")
    {
        if (content.length() < 1)
        {
            return;
        }
        this->logs->AppendText(fmt::format("{}", content));
    }
    if (token == "MESSAGE")
    {
        this->logs->AppendText(fmt::format("{}\n", content));
    }
}

void MainWindowUI::loadModelList()
{
    std::string oldSelection = this->m_model->GetStringSelection().ToStdString();

    this->LoadFileList(sd_gui_utils::DirTypes::CHECKPOINT);

    if (this->ModelFilesIndex.find(oldSelection) != this->ModelFilesIndex.end())
    {
        this->m_model->SetSelection(this->ModelFilesIndex[oldSelection]);
    }
}

void MainWindowUI::OnQueueItemManagerItemStatusChanged(QM::QueueItem item)
{
    auto store = this->m_joblist->GetStore();

    int lastCol = this->m_joblist->GetColumnCount() - 1;

    for (unsigned int i = 0; i < store->GetItemCount(); i++)
    {
        auto _item = store->GetItem(i);
        auto _item_data = store->GetItemData(_item);
        auto *_qitem = reinterpret_cast<QM::QueueItem *>(_item_data);
        if (_qitem->id == item.id)
        {
            store->SetValueByRow(wxVariant(QM::QueueStatus_str[item.status]), i, lastCol);
            this->m_joblist->Refresh();
            break;
        }
    }
}

void MainWindowUI::StartGeneration(QM::QueueItem myJob)
{

    if (myJob.mode == QM::GenerationMode::TXT2IMG)
    {
        std::thread *p = new std::thread(&MainWindowUI::GenerateTxt2img, this, this->GetEventHandler(), myJob);
        this->threads.emplace_back(p);
    }
    if (myJob.mode == QM::GenerationMode::IMG2IMG)
    {
        std::thread *p = new std::thread(&MainWindowUI::GenerateImg2img, this, this->GetEventHandler(), myJob);
        this->threads.emplace_back(p);
    }
}

void MainWindowUI::loadTypeList()
{
    this->m_type->Clear();
    unsigned int index = 0;
    for (auto type : sd_gui_utils::sd_type_gui_names)
    {
        if (type.second == "Default")
        {
            this->m_type->Select(index);
        }
        this->m_type->Append(type.second);
        index = 0;
    }

    // this->m_type->Select(sizeof(sd_gui_utils::sd_type_gui_names) - 1);
}

void MainWindowUI::ModelStandaloneHashingCallback(size_t readed_size, std::string sha256, void *custom_pointer)
{
    sd_gui_utils::VoidHolder *holder = static_cast<sd_gui_utils::VoidHolder *>(custom_pointer);

    wxEvtHandler *eventHandler = (wxEvtHandler *)holder->p1;
    sd_gui_utils::ModelFileInfo *modelinfo = (sd_gui_utils::ModelFileInfo *)holder->p2;
    modelinfo->hash_progress_size = readed_size;
    if (readed_size > 0)
    {
        wxThreadEvent *e = new wxThreadEvent();
        e->SetString("STANDALONE_HASHING_PROGRESS:placeholder");
        e->SetPayload(modelinfo);
        wxQueueEvent(eventHandler, e);
    }
}

void MainWindowUI::loadLoraList()
{
    this->LoadFileList(sd_gui_utils::DirTypes::LORA);
}

void MainWindowUI::ChangeGuiFromQueueItem(QM::QueueItem item)
{
    this->m_seed->SetValue(item.params.seed);
    this->m_width->SetValue(item.params.width);
    this->m_height->SetValue(item.params.height);
    this->m_steps->SetValue(item.params.sample_steps);
    this->m_clip_skip->SetValue(item.params.clip_skip);
    this->m_controlnetStrength->SetValue(item.params.control_strength);
    this->m_cfg->SetValue(item.params.cfg_scale);
    this->ChangeModelByName(item.model);

    int index = 0;
    for (auto taesd : this->TaesdFiles)
    {
        if (item.params.taesd_path == taesd.second)
        {
            this->m_taesd->Select(index);
            break;
        }
        index++;
    }
    index = 0;
    for (auto vae : this->VaeFiles)
    {
        if (item.params.vae_path == vae.second)
        {
            this->m_vae->Select(index);
            break;
        }
        index++;
    }
    index = 0;

    for (auto quant : sd_gui_utils::sd_type_gui_names)
    {
        if (item.params.wtype == quant.first)
        {
            this->m_type->Select(quant.first);
            break;
        }
    }

    this->m_vae_tiling->SetValue(item.params.vae_tiling);
    if (!item.params.control_image_path.empty())
    {
        if (std::filesystem::exists(item.params.control_image_path))
        {
            wxImage img;
            img.LoadFile(item.params.control_image_path);
            this->m_controlnetImagePreview->SetBitmap(img);
            this->m_controlnetImageOpen->SetPath(item.params.control_image_path);
        }
        index = 0;
        for (auto cnmodel : this->ControlnetModels)
        {
            if (cnmodel.second == item.params.controlnet_path)
            {
                this->m_controlnetModels->Select(index);
            }
            index++;
        }
    }
}

void MainWindowUI::ChangeModelByName(wxString ModelName)
{
    if (this->ModelFiles.find(ModelName.ToStdString()) != this->ModelFiles.end())
    {
        this->m_model->Select(this->ModelFilesIndex[ModelName.ToStdString()]);
        this->m_generate1->Enable();
    }
}

void MainWindowUI::LoadPresets()
{
    this->LoadFileList(sd_gui_utils::DirTypes::PRESETS);
}

sd_ctx_t *MainWindowUI::LoadModelv2(wxEvtHandler *eventHandler, QM::QueueItem myItem)
{
    wxThreadEvent *e = new wxThreadEvent();
    e->SetString(wxString::Format("MODEL_LOAD_START:%s", myItem.params.model_path));
    e->SetPayload(myItem);
    wxQueueEvent(eventHandler, e);
    bool model_exists = this->ModelManager->exists(myItem.params.model_path);
    if (!model_exists)
    {
        this->ModelManager->addModel(myItem.params.model_path, sd_gui_utils::DirTypes::CHECKPOINT, myItem.model);
    }
    // check if the model have a sha256
    if (model_exists)
    {
        auto model = this->ModelManager->getInfo(myItem.params.model_path);
        if (model.sha256.empty())
        {
            QM::QueueItem *item(&myItem);
            item->hash_fullsize = model.size;
            sd_gui_utils::VoidHolder *holder = new sd_gui_utils::VoidHolder;
            holder->p1 = (void *)eventHandler;
            holder->p2 = (void *)item;
            auto hash = sd_gui_utils::sha256_file_openssl(model.path.c_str(), (void *)holder, &MainWindowUI::ModelHashingCallback);
            this->ModelManager->setHash(myItem.params.model_path, hash);
        }
    }

    wxThreadEvent *ef = new wxThreadEvent();
    ef->SetString(wxString::Format("MODEL_LOAD_START:%s", myItem.params.model_path));
    ef->SetPayload(myItem);
    wxQueueEvent(eventHandler, ef);

    // std::lock_guard<std::mutex> guard(this->sdMutex);
    sd_ctx_t *sd_ctx_ = new_sd_ctx(
        sd_gui_utils::repairPath(myItem.params.model_path).c_str(),      // model path
        sd_gui_utils::repairPath(myItem.params.vae_path).c_str(),        // vae path
        sd_gui_utils::repairPath(myItem.params.taesd_path).c_str(),      // taesd path
        sd_gui_utils::repairPath(myItem.params.controlnet_path).c_str(), // controlnet path
        sd_gui_utils::repairPath(myItem.params.lora_model_dir).c_str(),  // lora path
        sd_gui_utils::repairPath(myItem.params.embeddings_path).c_str(), // embedding path
        false,                                                           // vae decode only (img2img = false)
        myItem.params.vae_tiling,                                        // vae tiling
        false,                                                           // free params immediatelly
        myItem.params.n_threads,
        myItem.params.wtype,
        myItem.params.rng_type,
        myItem.params.schedule,
        myItem.params.control_net_cpu);

    if (sd_ctx_ == NULL)
    {
        wxThreadEvent *c = new wxThreadEvent();
        c->SetString(wxString::Format("MODEL_LOAD_ERROR:%s", myItem.params.model_path));
        c->SetPayload(myItem);
        wxQueueEvent(eventHandler, c);
        this->modelLoaded = false;
        return nullptr;
    }
    else
    {
        wxThreadEvent *c = new wxThreadEvent();
        c->SetString(wxString::Format("MODEL_LOAD_DONE:%s", myItem.params.model_path));
        c->SetPayload(sd_ctx_);
        wxQueueEvent(eventHandler, c);
        this->modelLoaded = true;
        this->currentModel = myItem.params.model_path;
        this->currentVaeModel = myItem.params.vae_path;
        this->currentTaesdModel = myItem.params.taesd_path;
        this->currentwType = myItem.params.wtype;
        this->currentControlnetModel = myItem.params.controlnet_path;
    }
    return sd_ctx_;
}

void MainWindowUI::loadSamplerList()
{
    this->m_sampler->Clear();
    for (auto sampler : sd_gui_utils::sample_method_str)
    {
        int _u = this->m_sampler->Append(sampler);

        if (sampler == sd_gui_utils::sample_method_str[this->sd_params->sample_method])
        {
            this->m_sampler->Select(_u);
        }
    }
}

void MainWindowUI::initConfig()
{

    wxString datapath = wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "sd_ui_data" + wxFileName::GetPathSeparator();
    wxString imagespath = wxStandardPaths::Get().GetDocumentsDir() + wxFileName::GetPathSeparator() + "sd_ui_output" + wxFileName::GetPathSeparator();

    wxString model_path = datapath;
    model_path.append("checkpoints");

    wxString vae_path = datapath;
    vae_path.append("vae");

    wxString lora_path = datapath;
    lora_path.append("lora");

    wxString embedding_path = datapath;
    embedding_path.append("embedding");

    wxString taesd_path = datapath;
    taesd_path.append("taesd");

    wxString presets_path = datapath;
    presets_path.append("presets");

    wxString jobs_path = datapath;
    jobs_path.append("queue_jobs");

    wxString controlnet_path = datapath;
    controlnet_path.append("controlnet");

    this->cfg->datapath = datapath;

    this->cfg->lora = this->fileConfig->Read("/paths/lora", lora_path).ToStdString();
    this->cfg->model = this->fileConfig->Read("/paths/model", model_path).ToStdString();
    this->cfg->vae = this->fileConfig->Read("/paths/vae", vae_path).ToStdString();
    this->cfg->embedding = this->fileConfig->Read("/paths/embedding", embedding_path).ToStdString();
    this->cfg->taesd = this->fileConfig->Read("/paths/taesd", taesd_path).ToStdString();
    this->cfg->controlnet = this->fileConfig->Read("/paths/controlnet", controlnet_path).ToStdString();
    this->cfg->presets = this->fileConfig->Read("/paths/presets", presets_path).ToStdString();

    this->cfg->jobs = this->fileConfig->Read("/paths/jobs", jobs_path).ToStdString();

    this->cfg->output = this->fileConfig->Read("/paths/output", imagespath).ToStdString();
    this->cfg->keep_model_in_memory = this->fileConfig->Read("/keep_model_in_memory", this->cfg->keep_model_in_memory);
    this->cfg->save_all_image = this->fileConfig->Read("/save_all_image", this->cfg->save_all_image);
    this->cfg->n_threads = this->fileConfig->Read("/n_threads", cores());

    // populate data from sd_params as default...

    if (!this->modelLoaded && this->firstCfgInit)
    {
        this->m_cfg->SetValue(static_cast<double>(this->sd_params->cfg_scale));
        this->m_seed->SetValue(static_cast<int>(this->sd_params->seed));
        this->m_clip_skip->SetValue(this->sd_params->clip_skip);
        this->m_steps->SetValue(this->sd_params->sample_steps);
        this->m_width->SetValue(this->sd_params->width);
        this->m_height->SetValue(this->sd_params->height);
        this->m_batch_count->SetValue(this->sd_params->batch_count);
        this->loadSamplerList();
        this->loadTypeList();

        // check if directories exists...
        if (!std::filesystem::exists(model_path.ToStdString()))
        {
            std::filesystem::create_directories(model_path.ToStdString());
        }
        if (!std::filesystem::exists(lora_path.ToStdString()))
        {
            std::filesystem::create_directories(lora_path.ToStdString());
        }
        if (!std::filesystem::exists(vae_path.ToStdString()))
        {
            std::filesystem::create_directories(vae_path.ToStdString());
        }
        if (!std::filesystem::exists(embedding_path.ToStdString()))
        {
            std::filesystem::create_directories(embedding_path.ToStdString());
        }
        if (!std::filesystem::exists(taesd_path.ToStdString()))
        {
            std::filesystem::create_directories(taesd_path.ToStdString());
        }
        if (!std::filesystem::exists(presets_path.ToStdString()))
        {
            std::filesystem::create_directories(presets_path.ToStdString());
        }
        if (!std::filesystem::exists(jobs_path.ToStdString()))
        {
            std::filesystem::create_directories(jobs_path.ToStdString());
        }
        if (!std::filesystem::exists(controlnet_path.ToStdString()))
        {
            std::filesystem::create_directories(controlnet_path.ToStdString());
        }
        if (!std::filesystem::exists(imagespath.ToStdString()))
        {
            std::filesystem::create_directories(imagespath.ToStdString());
        }
    }
    this->firstCfgInit = false;
}

void MainWindowUI::OnExit(wxEvent &event)
{
    this->Close();
}

void MainWindowUI::OnQueueItemManagerItemUpdated(QM::QueueItem item)
{
}

MainWindowUI::~MainWindowUI()
{
    if (this->modelLoaded)
    {
        free_sd_ctx(this->sd_ctx);
    }
    for (auto &t : this->threads)
    {
        t->join();
        // t->~thread();
    }
    this->TaskBar->Destroy();
}

void MainWindowUI::OnPopupClick(wxCommandEvent &evt)
{
    // void *data = static_cast<wxMenu *>(evt.GetEventObject())->GetClientData();
    wxMenu *m = (wxMenu *)evt.GetClientObject();
    auto te = evt.GetInt();
    auto tu = evt.GetId();

    // 100 for queueitem list table
    if (tu < 100)
    {
        wxDataViewListStore *store = this->m_joblist->GetStore();
        auto currentItem = this->m_joblist->GetCurrentItem();
        auto currentRow = this->m_joblist->GetSelectedRow();

        QM::QueueItem *qitem = reinterpret_cast<QM::QueueItem *>(store->GetItemData(currentItem));

        /*
                1 Copy and queue
                2 copy to text2img
                3 copy prompts to text2image
                4 copy prompts to img2img
                5 Details
                99 delete
        */

        switch (tu)
        {
        case 1:
            this->qmanager->Duplicate(qitem->id);
        case 2:
            this->ChangeGuiFromQueueItem(*qitem);
            break;
        case 3:
            this->m_prompt->SetValue(qitem->params.prompt);
            this->m_neg_prompt->SetValue(qitem->params.negative_prompt);
            break;
        case 4:
            this->m_prompt2->SetValue(qitem->params.prompt);
            this->m_neg_prompt2->SetValue(qitem->params.negative_prompt);
            break;
        case 99:
            if (this->qmanager->DeleteJob(qitem->id))
            {
                store->DeleteItem(currentRow);
            }
            break;
        case 5:
            wxDisplay display(wxDisplay::GetFromWindow(this));
            wxRect screen = display.GetClientArea();
            MainWindowImageViewer *miv = new MainWindowImageViewer(this);
            miv->SetData(this->qmanager->GetItem(qitem->id));
            miv->SetSize(screen.GetSize());
            miv->SetPosition(wxPoint(0, 0));
            miv->SetThemeEnabled(true);
            miv->SetTitle(wxString::Format("Job details: %d", qitem->id));
            miv->Show();
            break;
        }
    }
    /*
    101   lora txt2img
    102   lora txt2img neg
    103   lora img2img
    104   lora img2img neg
    */
    if (tu >= 100)
    {
        wxDataViewListStore *store = this->m_data_model_list->GetStore();
        //        auto currentItem = this->m_data_model_list->GetCurrentItem();
        int currow = this->m_data_model_list->GetSelectedRow();
        auto currentItem = store->GetItem(currow);

        sd_gui_utils::ModelFileInfo *modelinfo = reinterpret_cast<sd_gui_utils::ModelFileInfo *>(store->GetItemData(currentItem));

        switch (tu)
        {
        case 100:
            this->threads.emplace_back(new std::thread(&MainWindowUI::threadedModelHashCalc, this, this->GetEventHandler(), modelinfo));
            break;
        case 101:
            this->m_prompt->SetValue(fmt::format("{} <lora:{}:0.5>", this->m_prompt->GetValue().ToStdString(), modelinfo->name));
            break;
        case 102:
            this->m_neg_prompt->SetValue(fmt::format("{} <lora:{}:0.5>", this->m_neg_prompt->GetValue().ToStdString(), modelinfo->name));
            break;
        case 103:
            this->m_prompt2->SetValue(fmt::format("{} <lora:{}:0.5>", this->m_prompt2->GetValue().ToStdString(), modelinfo->name));
            break;
        case 104:
            this->m_neg_prompt2->SetValue(fmt::format("{} <lora:{}:0.5>", this->m_neg_prompt2->GetValue().ToStdString(), modelinfo->name));
            break;
        case 200:
            this->ChangeModelByName(modelinfo->name);
            break;
        }
    }
}

void MainWindowUI::loadControlnetList()
{
    this->LoadFileList(sd_gui_utils::DirTypes::CONTROLNET);
}

void MainWindowUI::GenerateImg2img(wxEvtHandler *eventHandler, QM::QueueItem myItem)
{
    sd_gui_utils::VoidHolder *vparams = new sd_gui_utils::VoidHolder;
    vparams->p1 = (void *)this->GetEventHandler();
    vparams->p2 = (void *)&myItem;

    sd_set_progress_callback(MainWindowUI::HandleSDProgress, (void *)vparams);

    if (!this->modelLoaded)
    {
        this->sd_ctx = this->LoadModelv2(eventHandler, myItem);
        this->currentModel = myItem.params.model_path;
        this->currentVaeModel = myItem.params.vae_path;
        this->currentTaesdModel = myItem.params.taesd_path;
        this->currentwType = myItem.params.wtype;
        this->currentControlnetModel = myItem.params.controlnet_path;
    }
    else
    { // the model must be reloaded when: models is changed, vae is changed, taesd is changed
        if (myItem.params.model_path != this->currentModel ||
            this->currentVaeModel != myItem.params.vae_path ||
            this->currentTaesdModel != myItem.params.taesd_path ||
            this->currentwType != myItem.params.wtype)
        {
            free_sd_ctx(this->sd_ctx);
            this->sd_ctx = this->LoadModelv2(eventHandler, myItem);
        }
    }

    if (!this->modelLoaded || this->sd_ctx == nullptr)
    {
        wxThreadEvent *f = new wxThreadEvent();
        f->SetString("GENERATION_ERROR:Model load failed...");
        f->SetPayload(myItem);
        wxQueueEvent(eventHandler, f);
        return;
    }

    auto start = std::chrono::system_clock::now();

    sd_image_t *control_image = NULL;
    sd_image_t *results;

    if (std::filesystem::exists(myItem.initial_image))
    {
        int c = 0;
        int w, h;
        stbi_uc *input_image_buffer = stbi_load(myItem.initial_image.c_str(), &w, &h, &c, 3);
        control_image = new sd_image_t{(uint32_t)w, (uint32_t)h, 3, input_image_buffer};
        input_image_buffer = NULL;
        delete input_image_buffer;
    }
    else
    {
        wxThreadEvent *e = new wxThreadEvent();
        e->SetString(wxString::Format("GENERATION_ERROR:Can not open initial image: %s", myItem.initial_image));
        e->SetPayload(myItem);
        wxQueueEvent(eventHandler, e);
        return;
    }

    wxThreadEvent *e = new wxThreadEvent();
    e->SetString(wxString::Format("GENERATION_START:%s", this->sd_params->model_path));
    e->SetPayload(myItem);
    wxQueueEvent(eventHandler, e);

    results = img2img(this->sd_ctx,
                      *control_image,
                      myItem.params.prompt.c_str(),
                      myItem.params.negative_prompt.c_str(),
                      myItem.params.clip_skip,
                      myItem.params.cfg_scale,
                      myItem.params.width,
                      myItem.params.height,
                      myItem.params.sample_method,
                      myItem.params.sample_steps,
                      myItem.params.strength,
                      myItem.params.seed,
                      myItem.params.batch_count);

    control_image = NULL;
    delete control_image;
    if (results == NULL)
    {
        wxThreadEvent *f = new wxThreadEvent();
        f->SetString("GENERATION_ERROR:Something wrong happened at image generation...");
        f->SetPayload(myItem);
        wxQueueEvent(eventHandler, f);
        delete results;
        return;
    }
    /* save image(s) */

    const auto p1 = std::chrono::system_clock::now();
    auto ctime = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();

    for (int i = 0; i < this->sd_params->batch_count; i++)
    {
        if (results[i].data == NULL)
        {
            continue;
        }

        // handle data??
        wxImage *img = new wxImage(results[i].width, results[i].height, results[i].data);
        std::string filename = this->cfg->output;
        std::string extension = ".png";
        std::string filename_without_extension;
        filename = filename + wxFileName::GetPathSeparator();
        filename = filename + std::to_string(myItem.id);
        filename = filename + "_";
        filename = filename + std::to_string(myItem.params.seed + i);
        filename = filename + "_";

        if (this->sd_params->batch_count > 1)
        {
            filename = filename + std::to_string(ctime) + "_" + std::to_string(i);
        }
        else
        {
            filename = filename + std::to_string(ctime);
        }
        filename_without_extension = filename;
        filename = filename + extension;

        if (!img->SaveFile(filename))
        {
            wxThreadEvent *g = new wxThreadEvent();
            g->SetString(wxString::Format("GENERATION_ERROR:Failed to save image into %s", filename));
            g->SetPayload(myItem);
            wxQueueEvent(eventHandler, g);
        }
        else
        {
            myItem.images.emplace_back(filename);
            if (myItem.params.control_image_path.length() > 0 && this->cfg->save_all_image)
            {
                std::string ctrlFilename = this->cfg->output;
                ctrlFilename = filename_without_extension + "_ctrlimg_" + extension;
                wxImage _ctrlimg(myItem.params.control_image_path);
                _ctrlimg.SaveFile(ctrlFilename);
            }
        }

        // handle data??
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    // send to notify the user...
    wxThreadEvent *h = new wxThreadEvent();
    auto msg = fmt::format("MESSAGE:Image generation done in {}s. Saved into {}", elapsed_seconds.count(), this->cfg->output);
    h->SetString(wxString(msg.c_str()));
    // h->SetPayload(myItem);
    wxQueueEvent(eventHandler, h);

    /*  wxThreadEvent *i = new wxThreadEvent();
      i->SetString(wxString::Format("GENERATION_DONE:ok"));
      i->SetPayload(results);
      wxQueueEvent(eventHandler, i);*/

    // send to the queue manager
    wxThreadEvent *j = new wxThreadEvent();
    j->SetString(wxString::Format("QUEUE:%d", QM::QueueEvents::ITEM_FINISHED));
    j->SetPayload(myItem);
    wxQueueEvent(eventHandler, j);
}

void MainWindowUI::threadedModelHashCalc(wxEvtHandler *eventHandler, sd_gui_utils::ModelFileInfo *modelinfo)
{

    modelinfo->hash_fullsize = modelinfo->size;

    /* wxThreadEvent *e = new wxThreadEvent();
     e->SetString("STANDALONE_HASHING_PROGRESS:placeholder");
     e->SetPayload(modelinfo);
     wxQueueEvent(eventHandler, e);*/

    sd_gui_utils::VoidHolder *holder = new sd_gui_utils::VoidHolder;
    holder->p1 = (void *)eventHandler;
    holder->p2 = (void *)modelinfo;

    modelinfo->sha256 = sd_gui_utils::sha256_file_openssl(modelinfo->path.c_str(), (void *)holder, &MainWindowUI::ModelStandaloneHashingCallback);

    wxThreadEvent *r = new wxThreadEvent();
    r->SetString("STANDALONE_HASHING_DONE:placeholder");
    r->SetPayload(modelinfo);
    wxQueueEvent(eventHandler, r);
}

void MainWindowUI::GenerateTxt2img(wxEvtHandler *eventHandler, QM::QueueItem myItem)
{
    sd_gui_utils::VoidHolder *vparams = new sd_gui_utils::VoidHolder;
    vparams->p1 = (void *)this->GetEventHandler();
    vparams->p2 = (void *)&myItem;

    sd_set_progress_callback(MainWindowUI::HandleSDProgress, (void *)vparams);

    if (!this->modelLoaded)
    {
        this->sd_ctx = this->LoadModelv2(eventHandler, myItem);
        this->currentModel = myItem.params.model_path;
        this->currentVaeModel = myItem.params.vae_path;
        this->currentTaesdModel = myItem.params.taesd_path;
        this->currentwType = myItem.params.wtype;
        this->currentControlnetModel = myItem.params.controlnet_path;
    }
    else
    { // the model must be reloaded when: models is changed, vae is changed, taesd is changed
        if (myItem.params.model_path != this->currentModel ||
            this->currentVaeModel != myItem.params.vae_path ||
            this->currentTaesdModel != myItem.params.taesd_path ||
            this->currentwType != myItem.params.wtype ||
            this->currentControlnetModel != myItem.params.controlnet_path)
        {
            free_sd_ctx(this->sd_ctx);
            this->sd_ctx = this->LoadModelv2(eventHandler, myItem);
        }
        else
        {
            // it's a bug in sd.cpp, can not generate new image with controlnet, if already generated one...
            // they don't known, because the example sd.exe is just a one - time runner app... sadly...
            // so we need to destroy the ctx and re init it
            if (myItem.params.controlnet_path.length() > 0)
            {
                free_sd_ctx(this->sd_ctx);
                this->sd_ctx = this->LoadModelv2(eventHandler, myItem);
            }
        }
    }

    if (!this->modelLoaded || this->sd_ctx == nullptr)
    {
        wxThreadEvent *f = new wxThreadEvent();
        f->SetString("GENERATION_ERROR:Model load failed...");
        f->SetPayload(myItem);
        wxQueueEvent(eventHandler, f);
        return;
    }

    auto start = std::chrono::system_clock::now();

    wxThreadEvent *e = new wxThreadEvent();
    e->SetString(wxString::Format("GENERATION_START:%s", this->sd_params->model_path));
    e->SetPayload(myItem);
    wxQueueEvent(eventHandler, e);

    sd_image_t *control_image = NULL;
    sd_image_t *results;
    // prepare controlnet image, if have
    // if we have, but no model, the myItem will not contains any image
    if (myItem.params.control_image_path.length() > 0)
    {
        if (std::filesystem::exists(myItem.params.control_image_path))
        {
            int c = 0;
            int w, h;
            stbi_uc *input_image_buffer = stbi_load(myItem.params.control_image_path.c_str(), &w, &h, &c, 3);
            control_image = new sd_image_t{(uint32_t)w, (uint32_t)h, 3, input_image_buffer};
            input_image_buffer = NULL;
            delete input_image_buffer;
        }
    }
    // std::lock_guard<std::mutex> guard(this->sdMutex);

    results = txt2img(this->sd_ctx,
                      myItem.params.prompt.c_str(),
                      myItem.params.negative_prompt.c_str(),
                      myItem.params.clip_skip,
                      myItem.params.cfg_scale,
                      myItem.params.width,
                      myItem.params.height,
                      myItem.params.sample_method,
                      myItem.params.sample_steps,
                      myItem.params.seed,
                      myItem.params.batch_count,
                      control_image,
                      myItem.params.control_strength);

    control_image = NULL;
    delete control_image;
    if (results == NULL)
    {
        wxThreadEvent *f = new wxThreadEvent();
        f->SetString("GENERATION_ERROR:Something wrong happened at image generation...");
        f->SetPayload(myItem);
        wxQueueEvent(eventHandler, f);
        delete results;
        return;
    }
    /* save image(s) */

    const auto p1 = std::chrono::system_clock::now();
    auto ctime = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();

    for (int i = 0; i < myItem.params.batch_count; i++)
    {
        if (results[i].data == NULL)
        {
            continue;
        }

        // handle data??
        wxImage *img = new wxImage(results[i].width, results[i].height, results[i].data);
        std::string filename = this->cfg->output;
        // not handled from cfg
        // TODO: handle from cfg...
        std::string extension = ".png";
        std::string filename_without_extension;
        filename = filename + wxFileName::GetPathSeparator();
        filename = filename + std::to_string(myItem.id);
        filename = filename + "_";
        filename = filename + std::to_string(myItem.params.seed + i);
        filename = filename + "_";

        if (this->sd_params->batch_count > 1)
        {
            filename = filename + std::to_string(ctime) + "_" + std::to_string(i);
        }
        else
        {
            filename = filename + std::to_string(ctime);
        }
        filename_without_extension = filename;
        filename = filename + extension;

        // test
        img->SetOption("COM", myItem.params.prompt);
        img->SetOption("Comment", myItem.params.prompt);

        if (!img->SaveFile(filename))
        {
            wxThreadEvent *g = new wxThreadEvent();
            g->SetString(wxString::Format("GENERATION_ERROR:Failed to save image into %s", filename));
            g->SetPayload(myItem);
            wxQueueEvent(eventHandler, g);
        }
        else
        {
            myItem.images.emplace_back(filename);
            if (myItem.params.control_image_path.length() > 0 && this->cfg->save_all_image)
            {
                std::string ctrlFilename = this->cfg->output;
                ctrlFilename = filename_without_extension + "_ctrlimg_" + extension;
                wxImage _ctrlimg(myItem.params.control_image_path);
                _ctrlimg.SaveFile(ctrlFilename);
            }
        }

        // handle data??
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    // send to notify the user...
    wxThreadEvent *h = new wxThreadEvent();
    auto msg = fmt::format("MESSAGE:Image generation done in {}s. Saved into {}", elapsed_seconds.count(), this->cfg->output);
    h->SetString(wxString(msg.c_str()));
    // h->SetPayload(myItem);
    wxQueueEvent(eventHandler, h);

    /*wxThreadEvent *i = new wxThreadEvent();
    i->SetString(wxString::Format("GENERATION_DONE:ok"));
    i->SetPayload(results);
    wxQueueEvent(eventHandler, i);*/

    // send to the queue manager
    wxThreadEvent *j = new wxThreadEvent();
    j->SetString(wxString::Format("QUEUE:%d", QM::QueueEvents::ITEM_FINISHED));
    j->SetPayload(myItem);
    wxQueueEvent(eventHandler, j);
}

void MainWindowUI::OnQueueItemManagerItemAdded(QM::QueueItem item)
{
    wxVector<wxVariant> data;

    auto created_at = sd_gui_utils::formatUnixTimestampToDate(item.created_at);

    data.push_back(wxVariant(std::to_string(item.id)));
    data.push_back(wxVariant(created_at));
    data.push_back(wxVariant(sd_gui_utils::modes_str[item.mode]));
    data.push_back(wxVariant(item.model));
    data.push_back(wxVariant(sd_gui_utils::sample_method_str[(int)item.params.sample_method]));
    data.push_back(wxVariant(std::to_string(item.params.seed)));
    data.push_back(item.status == QM::QueueStatus::DONE ? 100 : 1); // progressbar
    data.push_back(wxString("-.--it/s"));                           // speed
    data.push_back(wxVariant(QM::QueueStatus_str[item.status]));    // status

    auto store = this->m_joblist->GetStore();

    QM::QueueItem *nItem = new QM::QueueItem(item);

    this->JobTableItems[item.id] = nItem;
    //  store->AppendItem(data, wxUIntPtr(this->JobTableItems[item.id]));
    store->PrependItem(data, wxUIntPtr(this->JobTableItems[item.id]));
}

void MainWindowUI::LoadFileList(sd_gui_utils::DirTypes type)
{
    std::string basepath;

    switch (type)
    {
    case sd_gui_utils::DirTypes::VAE:
        this->VaeFiles.clear();
        this->m_vae->Clear();
        this->m_vae->Append("-none-");
        this->m_vae->Select(0);
        basepath = this->cfg->vae;
        break;
    case sd_gui_utils::DirTypes::LORA:
        basepath = this->cfg->lora;
        break;
    case sd_gui_utils::DirTypes::CHECKPOINT:
        this->ModelFiles.clear();
        this->ModelFilesIndex.clear();
        this->m_model->Clear();
        this->m_model->Append("-none-");
        this->m_model->Select(0);
        basepath = this->cfg->model;
        break;
    case sd_gui_utils::DirTypes::PRESETS:
        this->Presets.clear();
        this->m_preset_list->Clear();
        this->m_preset_list->Append("-none-");
        this->m_preset_list->Select(0);
        basepath = this->cfg->presets;
        break;
    case sd_gui_utils::DirTypes::TAESD:
    {
        this->m_taesd->Clear();
        this->m_taesd->Append("-none-");
        this->m_taesd->Select(0);
        basepath = this->cfg->taesd;
    }
    break;
    case sd_gui_utils::DirTypes::CONTROLNET:
        this->m_controlnetModels->Clear();
        this->m_controlnetModels->Append("-none-");
        this->m_controlnetModels->Select(0);
        basepath = this->cfg->controlnet;
        break;
    }

    int i = 0;
    for (auto const &dir_entry : std::filesystem::recursive_directory_iterator(basepath))
    {
        if (!dir_entry.exists() || !dir_entry.is_regular_file() || !dir_entry.path().has_extension())
        {
            continue;
        }

        std::filesystem::path path = dir_entry.path();

        std::string ext = path.extension().string();

        if (type == sd_gui_utils::DirTypes::CHECKPOINT)
        {
            if (ext != ".safetensors" && ext != ".ckpt" && ext != ".gguf")
            {
                continue;
            }
        }
        if (type == sd_gui_utils::DirTypes::LORA)
        {
            if (ext != ".safetensors" && ext != ".ckpt" && ext != ".gguf")
            {
                continue;
            }
        }
        if (type == sd_gui_utils::DirTypes::VAE)
        {
            if (ext != ".safetensors" && ext != ".ckpt")
            {
                continue;
            }
        }

        if (type == sd_gui_utils::DirTypes::PRESETS)
        {
            if (ext != ".json")
            {
                continue;
            }
        }

        if (type == sd_gui_utils::DirTypes::TAESD)
        {
            if (ext != ".pth" && ext != ".safetensors" && ext != ".gguf")
            {
                continue;
            }
        }
        if (type == sd_gui_utils::DirTypes::CONTROLNET)
        {
            if (ext != ".safetensors" && ext != ".pth")
            {
                continue;
            }
        }
        std::string name = path.filename().replace_extension("").string();
        // prepend the subdirectory to the modelname
        // // wxFileName::GetPathSeparator()
        auto path_name = path.string();
        sd_gui_utils::replace(path_name, basepath, "");
        sd_gui_utils::replace(path_name, "//", "");
        sd_gui_utils::replace(path_name, "\\\\", "");
        sd_gui_utils::replace(path_name, ext, "");

        name = path_name.substr(1);

        if (type == sd_gui_utils::CHECKPOINT)
        {
            this->m_model->Append(name);
            this->ModelFiles.emplace(name, dir_entry.path().string());
            this->ModelFilesIndex[name] = this->ModelFiles.size();
            this->ModelManager->addModel(dir_entry.path().string(), type, name);
        }
        if (type == sd_gui_utils::LORA)
        {
            this->LoraFiles.emplace(name, dir_entry.path().string());
            this->ModelManager->addModel(dir_entry.path().string(), type, name);
        }
        if (type == sd_gui_utils::VAE)
        {
            this->m_vae->Append(name);
            this->VaeFiles.emplace(name, dir_entry.path().string());
        }
        if (type == sd_gui_utils::PRESETS)
        {
            sd_gui_utils::generator_preset preset;
            std::ifstream f(path.string());
            try
            {
                nlohmann::json data = nlohmann::json::parse(f);
                preset = data;
                preset.path = path.string();
                this->m_preset_list->Append(preset.name);
                this->Presets.emplace(preset.name, preset);
            }
            catch (const std::exception &e)
            {
                std::remove(path.string().c_str());
                std::cerr << e.what() << " file: " << path.string() << '\n';
            }
        }
        if (type == sd_gui_utils::TAESD)
        {
            this->m_taesd->Append(name);
            this->TaesdFiles.emplace(name, dir_entry.path().string());
        }
        if (type == sd_gui_utils::CONTROLNET)
        {
            this->m_controlnetModels->Append(name);
            this->ControlnetModels.emplace(name, dir_entry.path().string());
        }
    }

    if (type == sd_gui_utils::CHECKPOINT)
    {
        this->logs->AppendText(fmt::format("Loaded checkpoints: {}\n", this->ModelFiles.size()));
    }
    if (type == sd_gui_utils::LORA)
    {
        this->logs->AppendText(fmt::format("Loaded Loras: {}\n", this->LoraFiles.size()));
    }
    if (type == sd_gui_utils::VAE)
    {
        this->logs->AppendText(fmt::format("Loaded vaes: {}\n", this->VaeFiles.size()));
    }
    if (type == sd_gui_utils::PRESETS)
    {
        this->logs->AppendText(fmt::format("Loaded presets: {}\n", this->Presets.size()));
        if (this->Presets.size() > 0)
        {
            this->m_preset_list->Enable();
        }
    }
    if (type == sd_gui_utils::TAESD)
    {
        this->logs->AppendText(fmt::format("Loaded taesd: {}\n", this->TaesdFiles.size()));
    }
    if (type == sd_gui_utils::CONTROLNET)
    {
        this->logs->AppendText(fmt::format("Loaded controlnet: {}\n", this->ControlnetModels.size()));
    }
}

void MainWindowUI::HandleSDLog(sd_log_level_t level, const char *text, void *data)
{
    if (level == sd_log_level_t::SD_LOG_INFO || level == sd_log_level_t::SD_LOG_ERROR)
    {
        auto *eventHandler = (wxEvtHandler *)data;
        wxThreadEvent *e = new wxThreadEvent();
        e->SetString(wxString::Format("SD_MESSAGE:%s", text));
        e->SetPayload(level);
        wxQueueEvent(eventHandler, e);
    }
}

void MainWindowUI::loadTaesdList()
{

    this->LoadFileList(sd_gui_utils::DirTypes::TAESD);
}

void MainWindowUI::refreshModelTable(std::string filter, std::bitset<10> types)
{
    auto store = this->m_data_model_list->GetStore();

    if (!filter.empty())
    {
        std::transform(filter.begin(), filter.end(), filter.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });
    }

    auto list = this->ModelManager->getList();

    int curRow = 0;
    for (const auto &model : list)
    {
        auto name = model.name;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });

        if (filter.empty() || (!filter.empty() && name.find(filter) != std::string::npos))
        {
            if (types.none() || (!types.none() && types.test(model.model_type)))
            {
                wxVector<wxVariant> itemData;
                itemData.push_back(model.name);                                   // name
                itemData.push_back(model.size_f);                                 // size
                itemData.push_back(sd_gui_utils::dirtypes_str[model.model_type]); // type
                itemData.push_back(model.sha256);                                 // hash                                                                // hash
                itemData.push_back(model.sha256.empty() ? 0 : 100);               // progress bar
                store->AppendItem(itemData, (wxUIntPtr)this->ModelManager->getIntoPtr(model.path));
                curRow++;
            }
        }
    }
    auto col = this->m_data_model_list->GetColumn(0);
    col->SetSortable(true);
    auto col2 = this->m_data_model_list->GetColumn(1);
    col2->SetSortable(true);

    col->SetSortOrder(true);

    this->m_data_model_list->Refresh();
}

void MainWindowUI::HandleSDProgress(int step, int steps, float time, void *data)
{
    sd_gui_utils::VoidHolder *objs = (sd_gui_utils::VoidHolder *)data;
    wxEvtHandler *eventHandler = (wxEvtHandler *)objs->p1;
    QM::QueueItem *myItem = (QM::QueueItem *)objs->p2;
    myItem->step = step;
    myItem->steps = steps;
    myItem->time = time;
    /*
        format it/s
        time > 1.0f ? "\r%s %i/%i - %.2fs/it" : "\r%s %i/%i - %.2fit/s",
               progress.c_str(), step, steps,
               time > 1.0f || time == 0 ? time : (1.0f / time)
    */

    wxThreadEvent *e = new wxThreadEvent();
    e->SetString(wxString::Format("GENERATION_PROGRESS:%d/%d", step, steps));

    e->SetPayload(*myItem);
    wxQueueEvent(eventHandler, e);
}

void MainWindowUI::loadVaeList()
{

    this->LoadFileList(sd_gui_utils::DirTypes::VAE);
}

void MainWindowUI::OnCloseSettings(wxCloseEvent &event)
{
    this->initConfig();
    this->settingsWindow->Destroy();
}