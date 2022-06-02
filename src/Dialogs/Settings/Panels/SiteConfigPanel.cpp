/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "UtilsSettings.hpp"
#include "ConfigPanel.hpp"
#include "SiteConfigPanel.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Waypoint/Patterns.hpp"
#include "system/Path.hpp"
#include "io/CopyFile.hxx"

#include "Asset.hpp"

#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <vector>
#include <string>

#ifdef ANDROID
#include <android/log.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Android/Context.hpp"
#include "Android/Environment.hpp"
#include "Android/Main.hpp"
#endif

enum ControlIndex {
  DataPath,
  MapFile,
  WaypointFile,
  AdditionalWaypointFile,
  WatchedWaypointFile,
  AirspaceFile,
  AdditionalAirspaceFile,
  AirfieldFile,
  FlarmFile
};


class AllFileVisitor: public File::Visitor
{
    std::vector<Path> &list;

  public:
    AllFileVisitor(std::vector<Path> &_list):list(_list) {}

    void Visit(Path path, Path filename) override {
      list.emplace_back(path);
    }
};

class SiteConfigPanel final : public RowFormWidget {
  enum Buttons {
    WAYPOINT_EDITOR,
  };

  
  std::vector<Path> list;
  

public:
  SiteConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
  
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Copy();
};

void
SiteConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WndProperty *wp = Add(_T(""), 0, true);
  wp->SetText(GetPrimaryDataPath().c_str());
  wp->SetEnabled(false);

  AddFile(_("Map database"),
          _("The name of the file (.xcm) containing terrain, topography, and optionally "
            "waypoints, their details and airspaces."),
          ProfileKeys::MapFile, _T("*.xcm\0*.lkm\0"), FileType::MAP);

  AddFile(_("Waypoints"),
          _("Primary waypoints file.  Supported file types are Cambridge/WinPilot files (.dat), "
            "Zander files (.wpz) or SeeYou files (.cup)."),
          ProfileKeys::WaypointFile, WAYPOINT_FILE_PATTERNS,
          FileType::WAYPOINT);

  AddFile(_("More waypoints"),
          _("Secondary waypoints file.  This may be used to add waypoints for a competition."),
          ProfileKeys::AdditionalWaypointFile, WAYPOINT_FILE_PATTERNS,
          FileType::WAYPOINT);
  SetExpertRow(AdditionalWaypointFile);

  AddFile(_("Watched waypoints"),
          _("Waypoint file containing special waypoints for which additional computations like "
            "calculation of arrival height in map display always takes place. Useful for "
            "waypoints like known reliable thermal sources (e.g. powerplants) or mountain passes."),
          ProfileKeys::WatchedWaypointFile, WAYPOINT_FILE_PATTERNS,
          FileType::WAYPOINT);
  SetExpertRow(WatchedWaypointFile);

  AddFile(_("Airspaces"), _("The file name of the primary airspace file."),
          ProfileKeys::AirspaceFile, _T("*.txt\0*.air\0*.sua\0"),
          FileType::AIRSPACE);

  AddFile(_("More airspaces"), _("The file name of the secondary airspace file."),
          ProfileKeys::AdditionalAirspaceFile, _T("*.txt\0*.air\0*.sua\0"),
          FileType::AIRSPACE);
  SetExpertRow(AdditionalAirspaceFile);

  AddFile(_("Waypoint details"),
          _("The file may contain extracts from enroute supplements or other contributed "
            "information about individual waypoints and airfields."),
          ProfileKeys::AirfieldFile, _T("*.txt\0"),
          FileType::WAYPOINTDETAILS);
  SetExpertRow(AirfieldFile);

  AddFile(_("FLARM Device Database"),
          _("The name of the file containing information about registered FLARM devices."),
          ProfileKeys::FlarmFile, _T("*.fln\0"),
          FileType::FLARMNET);

}


void
SiteConfigPanel::Show(const PixelRect &rc) noexcept
{
  ConfigPanel::BorrowExtraButton(1, _("CopyFiles"), [this](){
    Copy();
  });

  RowFormWidget::Show(rc);
}

void
SiteConfigPanel::Hide() noexcept
{
  RowFormWidget::Hide();
  ConfigPanel::ReturnExtraButton(1);
}

bool
SiteConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  MapFileChanged = SaveValueFileReader(MapFile, ProfileKeys::MapFile);

  // WaypointFileChanged has already a meaningful value
  WaypointFileChanged |= SaveValueFileReader(WaypointFile, ProfileKeys::WaypointFile);
  WaypointFileChanged |= SaveValueFileReader(AdditionalWaypointFile, ProfileKeys::AdditionalWaypointFile);
  WaypointFileChanged |= SaveValueFileReader(WatchedWaypointFile, ProfileKeys::WatchedWaypointFile);

  AirspaceFileChanged = SaveValueFileReader(AirspaceFile, ProfileKeys::AirspaceFile);
  AirspaceFileChanged |= SaveValueFileReader(AdditionalAirspaceFile, ProfileKeys::AdditionalAirspaceFile);

  FlarmFileChanged = SaveValueFileReader(FlarmFile, ProfileKeys::FlarmFile);

  AirfieldFileChanged = SaveValueFileReader(AirfieldFile, ProfileKeys::AirfieldFile);


  changed = WaypointFileChanged || AirfieldFileChanged || MapFileChanged || FlarmFileChanged;

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateSiteConfigPanel()
{
  return std::make_unique<SiteConfigPanel>();
}

void
SiteConfigPanel::Copy()
{
  

/* Android: ask the Android API */
  if constexpr (IsAndroid()) {
    
  list.clear();

  AllFileVisitor afv(list);
#ifdef ANDROID
    const auto env = Java::GetEnv();

    for (auto &path : context->GetExternalFilesDirs(env)) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Context.getExternalFilesDirs()='%s'",
                          path.c_str());
      
    }
    __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Context.getExternalFilesDir_s()='%s'",
                          context->GetExternalFilesDir(env).c_str());

    if (auto path = Environment::GetExternalStoragePublicDirectory(env,
                                                                   "XCSoarData");
        path != nullptr) {
      const bool writeable = access(path.c_str(), W_OK) == 0;
      const bool readable = access(Path(_T("/storage/emulated/0/XCSoarData")).c_str(), R_OK) == 0;
      if(!writeable){
        __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                            "Environment.getExternalStoragePublicDirectory()='%s'%s",
                            path.c_str(),
                            writeable ? "" : " (not accessible)");
        __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                            "/storage/emulated/0/XCSoarData %s",
                            readable ? "" : " (read not accessible)");
        
        Directory::VisitFiles(path, afv, true);
        std::string pathString = path.c_str();
        for(unsigned i = 0; i < list.size(); i++){
          std::string filename = list[i].c_str();
          __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                            "Environment.getExternalStoragePublicDirectory()='%s'%s",
                            path.c_str(),
                            filename.substr(pathString.length(), filename.length()).c_str());
          std::string dest = context->GetExternalFilesDir(env).c_str();
          dest.append(filename.substr(pathString.length(), filename.length()));
          const Path src_filename = Path(list[i].c_str());
          const Path dest_filename = Path(dest.c_str());
          CopyFile(src_filename, dest_filename);
          __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                            "source %s dest %s",
                            src_filename.c_str(),
                            dest_filename.c_str());
        }
      }
                          
    }
#endif

  }
}