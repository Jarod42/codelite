#include "property_grid_wrapper.h"

#include "StdToWX.h"
#include "allocator_mgr.h"
#include "bool_property.h"
#include "choice_property.h"
#include "color_property.h"
#include "font_property.h"
#include "multi_strings_property.h"
#include "string_property.h"
#include "wxgui_helpers.h"

#define PROP_PG_BOOL_VALUE "Bool Value"
#define PROP_PG_STRING_VALUE "String Value"
#define PROP_PG_CHOICES PROP_OPTIONS
#define PROP_PG_ENUM_CHOICES PROP_OPTIONS
#define PROP_PG_CHOICES_VALUES "Array Integer Values"
#define PROP_PG_WILDCARD "Wildcard"
#define PROP_PG_COLOUR_DEFAULT "Initial Colour"

PropertyGridWrapper::PropertyGridWrapper()
    : wxcWidget(ID_WXPGPROPERTY)
{
    m_properties.Clear();
    m_sizerFlags.Clear();
    m_styles.Clear();
    static int labelCount = 0;

    const wxArrayString customEditors = StdToWX::ToArrayString({
        "",                  // 0
        "TextCtrl",          // 1
        "Choice",            // 2
        "ComboBox",          // 3
        "CheckBox",          // 4
        "TextCtrlAndButton", // 5 (default)
        "ChoiceAndButton",   // 6
        "SpinCtrl",          // 7
        "DatePickerCtrl",    // 8
    });

    SetPropertyString(_("Common Settings"), "wxPropertyGrid");
    Add<CategoryProperty>("wxPGProperty");
    Add<StringProperty>(PROP_NAME, "", _("The property name"));
    Add<StringProperty>(PROP_LABEL, wxString("My Label") << ++labelCount, _("The property label"));
    Add<MultiStringsProperty>(PROP_TOOLTIP, _("The property help string"));
    Add<ColorProperty>(PROP_BG, "<Default>", _("Property background colour"));
    Add<ChoiceProperty>(PROP_CUSTOM_EDITOR, customEditors, 0, _("Set custom editor control to a property"));

    const wxArrayString kindArr = StdToWX::ToArrayString({
        "wxPropertyCategory",     // 0
        "wxIntProperty",          // 1
        "wxFloatProperty",        // 2
        "wxBoolProperty",         // 3
        "wxStringProperty",       // 4 (default)
        "wxLongStringProperty",   // 5
        "wxDirProperty",          // 6
        "wxArrayStringProperty",  // 7
        "wxFileProperty",         // 8
        "wxEnumProperty",         // 9
        "wxEditEnumProperty",     // 10
        "wxFlagsProperty",        // 11
        "wxDateProperty",         // 12
        "wxImageFileProperty",    // 13
        "wxFontProperty",         // 14
        "wxSystemColourProperty", // 15
    });

    Add<ChoiceProperty>(PROP_KIND, kindArr, 4, _("The property kind"));

    Add<CategoryProperty>("wxPGProperty");
    Add<StringProperty>(PROP_PG_STRING_VALUE, "", _("Initial string value"));
    Add<MultiStringsProperty>(PROP_PG_CHOICES,
                              _("For properties that accepts array of strings (wxEnumProperty, "
                                "wxEditEnumProperty and wxFlagsProperty) set the strings display name"));
    Add<MultiStringsProperty>(
        PROP_PG_CHOICES_VALUES,
        _("For properties that accepts array of strings (wxEnumProperty, wxEditEnumProperty and wxFlagsProperty) set "
          "the strings values (optional for wxEnumProperty and wxEditEnumProperty)"));

    Add<CategoryProperty>("wxBoolProperty");
    Add<BoolProperty>(PROP_PG_BOOL_VALUE, true, _("Initial value, relevant to wxBoolProperty"));

    Add<CategoryProperty>("wxFileProperty");
    Add<StringProperty>(PROP_PG_WILDCARD, "", _("wxFileProperty's wildcard"));

    Add<CategoryProperty>("wxFontProperty");
    Add<FontProperty>(PROP_FONT, "", _("Initial font"));

    Add<CategoryProperty>("wxSystemColourProperty");
    Add<ColorProperty>(PROP_PG_COLOUR_DEFAULT, "<Default>", _("Initial Colour"));

    m_namePattern = "m_pgProp";
    SetName(GenerateName());
}

wxcWidget* PropertyGridWrapper::Clone() const { return new PropertyGridWrapper(); }

wxString PropertyGridWrapper::CppCtorCode() const
{
    wxString grid = GetWindowParent();
    wxString appendFunc = "Append( ";
    if(GetParent()->GetType() == ID_WXPGPROPERTY) {
        appendFunc = "AppendIn( " + GetParent()->GetName() + ", ";
    }

    wxString cppCode;

    wxString proptype = PropertyString(PROP_KIND);
    wxString strvalue;
    wxString arrName = grid + "Arr";
    wxString arrIntName = grid + "IntArr";
    wxString propName = GetName();

    if(proptype == "wxBoolProperty") {
        strvalue = PropertyString(PROP_PG_BOOL_VALUE);

    } else if(proptype == "wxIntProperty" || proptype == "wxFloatProperty") {
        strvalue = PropertyString(PROP_PG_STRING_VALUE);
        strvalue.Trim().Trim(false);
        if(strvalue.IsEmpty()) {
            strvalue = "0";
        }

    } else if(proptype == "wxFlagsProperty") {
        strvalue = PropertyString(PROP_PG_STRING_VALUE);
        strvalue.Trim().Trim(false);
        if(strvalue.IsEmpty()) {
            strvalue = "0";
        }

    } else {
        strvalue = wxCrafter::UNDERSCORE(PropertyString(PROP_PG_STRING_VALUE));
    }

    if(proptype == "wxIntProperty" || proptype == "wxFloatProperty" || proptype == "wxBoolProperty" ||
       proptype == "wxStringProperty" || proptype == "wxLongStringProperty" || proptype == "wxDirProperty" ||
       proptype == "wxFileProperty") {

        cppCode << propName << " = " << grid << "->" << appendFunc << " new " << proptype << "( "
                << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ", wxPG_LABEL, " << strvalue << ") );\n";

        if(proptype == "wxFileProperty") {
            cppCode << "#if !defined(__WXOSX__) && !defined(_WIN64)\n";
            cppCode << propName << "->SetAttribute(wxPG_FILE_WILDCARD, "
                    << wxCrafter::WXT(PropertyString(PROP_PG_WILDCARD)) << ");\n";
            cppCode << "#endif // !defined(__WXOSX__) && !defined(_WIN64)\n";
            cppCode << propName << "->SetAttribute(\"ShowFullPath\", 1);\n";
        }

    } else if(proptype == "wxPropertyCategory") {
        cppCode << propName << " = " << grid << "->" << appendFunc << " new wxPropertyCategory( "
                << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << " ) );\n";

    } else if(proptype == "wxArrayStringProperty") {
        wxArrayString options = wxCrafter::Split(PropertyString(PROP_PG_CHOICES), ";");
        cppCode << arrName << ".Clear();\n";
        for(size_t i = 0; i < options.GetCount(); ++i) {
            cppCode << arrName << ".Add(" << wxCrafter::UNDERSCORE(options.Item(i)) << ");\n";
        }
        cppCode << propName << " = " << grid << "->" << appendFunc << " new " << proptype << "( "
                << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ", wxPG_LABEL, " << arrName << ") );\n";

    } else if(proptype == "wxEnumProperty") {
        wxArrayString options = wxCrafter::Split(PropertyString(PROP_PG_CHOICES), ";");
        cppCode << arrName << ".Clear();\n";
        cppCode << arrIntName << ".Clear();\n";
        size_t selection = 0;
        for(size_t i = 0; i < options.GetCount(); ++i) {
            cppCode << arrName << ".Add(" << wxCrafter::UNDERSCORE(options.Item(i)) << ");\n";
            if(wxCrafter::UNDERSCORE(options.Item(i)) == strvalue) {
                selection = i;
            }
        }

        options = wxCrafter::Split(PropertyString(PROP_PG_CHOICES_VALUES), ";");
        for(size_t i = 0; i < options.GetCount(); ++i) {
            cppCode << arrIntName << ".Add(" << options.Item(i) << ");\n";
        }

        cppCode << propName << " = " << grid << "->" << appendFunc << " new " << proptype << "( "
                << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ", wxPG_LABEL, " << arrName << ", "
                << arrIntName << ", " << selection << ") );\n";

    } else if(proptype == "wxEditEnumProperty") {
        wxArrayString options = wxCrafter::Split(PropertyString(PROP_PG_CHOICES), ";");
        cppCode << arrName << ".Clear();\n";
        cppCode << arrIntName << ".Clear();\n";
        for(size_t i = 0; i < options.GetCount(); ++i) {
            cppCode << arrName << ".Add(" << wxCrafter::UNDERSCORE(options.Item(i)) << ");\n";
        }

        options = wxCrafter::Split(PropertyString(PROP_PG_CHOICES_VALUES), ";");
        for(size_t i = 0; i < options.GetCount(); ++i) {
            cppCode << arrIntName << ".Add(" << options.Item(i) << ");\n";
        }

        cppCode << propName << " = " << grid << "->" << appendFunc << " new " << proptype << "( "
                << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ", wxPG_LABEL, " << arrName << ", "
                << arrIntName << ", " << strvalue << ") );\n";

    } else if(proptype == "wxFlagsProperty") {
        wxArrayString options = wxCrafter::Split(PropertyString(PROP_PG_CHOICES), ";");
        cppCode << arrName << ".Clear();\n";
        cppCode << arrIntName << ".Clear();\n";
        for(size_t i = 0; i < options.GetCount(); ++i) {
            cppCode << arrName << ".Add(" << wxCrafter::UNDERSCORE(options.Item(i)) << ");\n";
        }

        options = wxCrafter::Split(PropertyString(PROP_PG_CHOICES_VALUES), ";");
        for(size_t i = 0; i < options.GetCount(); ++i) {
            cppCode << arrIntName << ".Add(" << options.Item(i) << ");\n";
        }

        cppCode << propName << " = " << grid << "->" << appendFunc << " new " << proptype << "( "
                << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ", wxPG_LABEL, " << arrName << ", "
                << arrIntName << ", " << strvalue << ") );\n";

    } else if(proptype == "wxDateProperty") {
        cppCode << propName << " = " << grid << "->" << appendFunc << " new wxDateProperty( "
                << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ", wxPG_LABEL, wxDateTime::Now()) );\n";

    } else if(proptype == "wxImageFileProperty") {
        cppCode << propName << " = " << grid << "->" << appendFunc << " new wxImageFileProperty( "
                << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ") );\n";

    } else if(proptype == "wxFontProperty") {
        wxString fontMemberName = propName + "Font";
        wxString font = wxCrafter::FontToCpp(PropertyString(PROP_FONT), fontMemberName);
        if(!font.IsEmpty() && font != "wxNullFont") {
            cppCode << font;
            cppCode << propName << " = " << grid << "->" << appendFunc << " new wxFontProperty( "
                    << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ", wxPG_LABEL, " << fontMemberName
                    << ") );\n";

        } else {
            cppCode << propName << " = " << grid << "->" << appendFunc << " new wxFontProperty( "
                    << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ") );\n";
            cppCode << propName << "->SetValueToUnspecified();\n";
        }
    } else if(proptype == "wxSystemColourProperty") {
        wxString colorname = wxCrafter::ColourToCpp(PropertyString(PROP_PG_COLOUR_DEFAULT));
        if(!colorname.IsEmpty()) {
            cppCode << propName << " = " << grid << "->" << appendFunc << " new wxSystemColourProperty( "
                    << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ", wxPG_LABEL, " << colorname << ") );\n";
        } else {
            cppCode << propName << " = " << grid << "->" << appendFunc << " new wxSystemColourProperty( "
                    << wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)) << ") );\n";
            cppCode << propName << "->SetValueToUnspecified();\n";
        }
    }

    if(!cppCode.IsEmpty()) {
        // set common properties
        cppCode << propName << "->SetHelpString(" << wxCrafter::UNDERSCORE(PropertyString(PROP_TOOLTIP)) << ");\n";
    }

    wxString propEditor = PropertyString(PROP_CUSTOM_EDITOR);
    if(!propEditor.IsEmpty()) {
        cppCode << propName << "->SetEditor( " << wxCrafter::WXT(propEditor) << " );\n";
    }
    return cppCode;
}

void PropertyGridWrapper::GetIncludeFile(wxArrayString& headers) const
{
    headers.Add("#include <wx/propgrid/property.h>");
    headers.Add("#include <wx/propgrid/advprops.h>");
}

wxString PropertyGridWrapper::GetWxClassName() const { return "wxPGProperty"; }

void PropertyGridWrapper::ToXRC(wxString& text, XRC_TYPE type) const
{
    if(type == XRC_LIVE) {
        return;

    } else {
        wxString value = wxCrafter::CDATA(PropertyString(PROP_PG_STRING_VALUE));
        if(PropertyString(PROP_KIND) == "wxBoolProperty") {
            value = PropertyString(PROP_PG_BOOL_VALUE);
        }

        text << "<object class=\"wxPGProperty\">"
             << "<proptype>" << PropertyString(PROP_KIND) << "</proptype>"
             << "<label>" << wxCrafter::CDATA(PropertyString(PROP_LABEL)) << "</label>"
             << "<value>" << value << "</value>"
             << "<wildcard>" << wxCrafter::CDATA(PropertyString(PROP_PG_WILDCARD)) << "</wildcard>"
             << "<editor>" << wxCrafter::CDATA(PropertyString(PROP_CUSTOM_EDITOR)) << "</editor>"
             << XRCCommonAttributes() << XRCContentItems();
        ChildrenXRC(text, type);
        text << XRCSuffix();
    }
}

bool PropertyGridWrapper::IsValidParent() const { return false; }

bool PropertyGridWrapper::IsWxWindow() const { return false; }

wxString PropertyGridWrapper::DoGenerateCppCtorCode_End() const
{
    // We set the colour attribute here since it needs to be placed after
    // all children have been added
    wxString cppCode;
    wxString colorname = wxCrafter::ColourToCpp(PropertyString(PROP_BG));
    if(!colorname.IsEmpty()) {
        cppCode << GetName() << "->SetBackgroundColour(" << colorname << ");\n";
    }
    return cppCode;
}
