#ifndef DIDCHANGE_TEXTDOCUMENTREQUEST_H
#define DIDCHANGE_TEXTDOCUMENTREQUEST_H

#include "LSP/Notification.h"

#include <wx/string.h>

namespace LSP
{

class WXDLLIMPEXP_CL DidChangeTextDocumentRequest : public LSP::Notification
{
public:
    explicit DidChangeTextDocumentRequest(const wxString& filename, const wxString& fileContent);
    virtual ~DidChangeTextDocumentRequest();
};

}; // namespace LSP

#endif // DIDCHANGE_TEXTDOCUMENTREQUEST_H
