#pragma once

#define MOVIEMAN_FADE_DURATION      1300
#define MOVIEMAN_SKIPPABLE_YES      true
#define MOVIEMAN_POSTEFFECT_FADEOUT 1

class CMovieMan : public CSingleton<CMovieMan>
{
public:
    CMovieMan() = default;
    virtual ~CMovieMan() = default;

    void ClearToBlack();
    void PlayLogo(const char* pcszName);
    void PlayIntro();
    BOOL PlayTutorial(LONG nIdx);

private:
    BOOL PlayMovie(const char* cpFileName, bool bSkipAllowed = false, int nPostEffectID = 0, DWORD dwPostEffectData = 0);
};
