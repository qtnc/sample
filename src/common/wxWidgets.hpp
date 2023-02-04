#ifndef _____WXW0_____INCL0
#define _____WXW0_____INCL0
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
 #include <wx/wx.h>
#endif
#include<string>
#include<iosfwd>
#include <fmt/ostream.h>

#define translate(S) app.lang.get(S,S)

#ifdef __WIN32
#undef CreateFile
#undef MessageBoxEx
#endif

struct App;
wxDECLARE_APP(App);

void setClipboardText (const std::string& s);
std::string getClipboardText ();

inline std::string U (const wxString& s) {
return s.ToStdString(wxConvUTF8);
}

inline std::string UFN (const wxString& s) {
return s.ToStdString(*wxConvFileName);
}

inline wxString U (const std::string& s) {
return wxString(s.data(), wxConvUTF8, s.size());
}

inline wxString U (const char* s) {
return s? wxString::FromUTF8(s) : wxString(wxEmptyString);
}

inline wxString UI (const std::string& s) {
wxString r(s.data(), wxConvUTF8, s.size());
if (r.size()==0 && s.size()!=0) r = wxString(s.data(), wxConvLocal, s.size());
if (r.size()==0 && s.size()!=0) r = s;
return r;
}

template<class F> struct finally {
F f;
finally (const F& x): f(x) {}
~finally () { f(); }
};

template <> struct fmt::formatter<wxString> : ostream_formatter {};

#endif
