#include "StdAfx.h"
#include "MovieMan.h"
#include "PythonApplication.h"

void CMovieMan::ClearToBlack()
{
}

void CMovieMan::PlayLogo(const char* pcszName)
{
    if (!pcszName || !*pcszName)
        return;

    CPythonApplication::Instance().OnLogoOpen(const_cast<char*>(pcszName));

    while (CPythonApplication::Instance().OnLogoUpdate())
    {
        CPythonApplication::Instance().Process();
        CPythonApplication::Instance().RenderGame();
        CPythonApplication::Instance().OnLogoRender();
    }

    CPythonApplication::Instance().OnLogoClose();
}

void CMovieMan::PlayIntro()
{
}

BOOL CMovieMan::PlayTutorial(LONG nIdx)
{
    return FALSE;
}

BOOL CMovieMan::PlayMovie(const char* cpFileName, bool bSkipAllowed, int nPostEffectID, DWORD dwPostEffectData)
{
    return FALSE;
}
