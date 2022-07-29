// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __SERIALIZE_GLOBAL_H__
#define __SERIALIZE_GLOBAL_H__
#include <cereal/cereal.hpp>

class SerializeGlobal
{
	int dramWait, cpuScale, joystick1;
	bool bSoundOn;

public:
#ifdef _ANDROID
	template<class Archive>
	inline void save(Archive & ar, std::uint32_t const version) const
	{
        switch (version) {
            case 1:
                ar(g_cpuScale, g_dramWait, g_bSoundOn);
                break;
            case 2:
                ar(g_cpuScale, g_dramWait, g_bSoundOn, (int)g_joystick1);
                break;
            default:
                break;
        }
	}

	template<class Archive>
	inline void load(Archive & ar, std::uint32_t const version)
	{
        switch (version) {
            case 1:
                ar(cpuScale, dramWait, bSoundOn);
                g_cpuScale = cpuScale;
                g_dramWait = dramWait;
                g_bSoundOn = bSoundOn;
                break;
            case 2:
                ar(cpuScale, dramWait, bSoundOn, joystick1);
                g_cpuScale = cpuScale;
                g_dramWait = dramWait;
                g_bSoundOn = bSoundOn;
                g_joystick1 = (unsigned char)joystick1;
                break;
            default:
                break;

        }
	}
#else
    template<class Archive>
	inline void save(Archive & ar, std::uint32_t const version) const
	{
		ar(g_cpuScale, g_dramWait, g_bSoundOn);
	}

	template<class Archive>
	inline void load(Archive & ar, std::uint32_t const version)
	{
		if (version < 2) throw(g_strTable[(int)Msg::The_state_file_is_old_and_cant_be_used]);

		ar(cpuScale, dramWait, bSoundOn);
		g_cpuScale = cpuScale;
		g_dramWait = dramWait;
		g_bSoundOn = bSoundOn;
	}
#endif
};

CEREAL_CLASS_VERSION(SerializeGlobal, CEREAL_VER);

#endif