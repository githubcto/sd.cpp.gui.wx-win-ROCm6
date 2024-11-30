#ifndef __MainWindowSettings__
#define __MainWindowSettings__

/**
@file
Subclass of Settings, which is generated by wxFormBuilder.
*/

/** Implementing Settings */

class MainWindowSettings : public Settings {
protected:
    // Handlers for Settings events.
    void OnImgQualityScroll(wxScrollEvent& event) override;
    void OnPngCompressionScroll(wxScrollEvent& event) override;
    void onShowNotificationCheck(wxCommandEvent& event) override;
    void OnCivitaiHelpButton(wxCommandEvent& event) override;
    void onSave(wxCommandEvent& event) override;
    void OnTAESDHelpClick(wxCommandEvent& event) override;
    void OnOutputFileNameFormatHelpClick(wxCommandEvent& event) override;
    void OnOpenFolder(wxCommandEvent& event) override;

public:
    /** Constructor */
    MainWindowSettings(wxWindow* parent);
    ~MainWindowSettings();
    //// end generated class members
private:
    void InitConfig();
    wxFileConfig* fileConfig;
    std::shared_ptr<sd_gui_utils::config> cfg;
    std::string ini_path;
    std::map<int, std::string> locales;
};

#endif  // __MainWindowSettings__
