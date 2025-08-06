#include "fmtShfmtFormat.hpp"

#include "Platform/Platform.hpp"

fmtShfmtFormat::fmtShfmtFormat()
{
    SetName("shfmt");
    SetFileTypes({ FileExtManager::TypeShellScript });
    SetDescription(_("shfmt formats shell programs"));
    SetShortDescription(_("shfmt formats shell programs"));
    SetConfigFilepath(wxEmptyString);
    SetInplaceFormatter(false);

    // local command
    const auto shfmt_exe = ThePlatform->Which("shfmt");
    SetCommand({ shfmt_exe.value_or("shfmt"), R"#("$(CurrentFileRelPath)")#" });
    SetEnabled(shfmt_exe.has_value());
}

fmtShfmtFormat::~fmtShfmtFormat() {}
