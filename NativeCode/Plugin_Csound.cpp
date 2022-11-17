#include "AudioPluginUtil.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#ifdef WIN32
#include "windows.h"
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#else
#include <dlfcn.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <csound.hpp>

namespace CsoundPlugin
{
    #define MIN 0
    #define MAX 1
    #define VALUE 2
    std::string csdFilename = "";
    bool debugMode = false;
    int numParams;
    struct CsoundChannel
    {
        float range[3];
        std::string name, text, label, caption, type;
    };

    std::vector<CsoundChannel> csoundChannels;


    //========================================================================
    // Simple Csound class that handles compiling of Csound and generating audio
    //========================================================================
    struct CsoundUnity
    {
        CsoundUnity();
        void process(float* outbuffer, float* inbuffer, unsigned int length, int channels);
        std::unique_ptr<Csound> csound;
        int csoundReturnCode;
        int ksmpsIndex = 0;
        int ksmps;
        MYFLT cs_scale;
        MYFLT *csoundInput, *csoundOutput;

        struct Data
        {
            float p[100];
        };
        union
        {
            Data data;
            unsigned char pad[(sizeof(Data) + 15) & ~15]; // This entire structure must be a multiple of 16 bytes (and and instance 16 byte aligned) for PS3 SPU DMA requirements
        };

        int sampleIndex = 0;
        int CompileCsound(std::string csdFile);

        bool csoundCompileOk()
        {
            if (csoundReturnCode == 0)
                return true;
            else
                return false;
        }

    private:

    };

    CsoundUnity::CsoundUnity()
    {

    }

    void CsoundUnity::process(float* outbuffer, float* inbuffer, unsigned int length, int channels)
    {
        if (csoundCompileOk())
        {
            unsigned int samples = length;
            unsigned int position = 0;
            while (samples--)
            {
                if (ksmpsIndex >= ksmps)
                {
                    csound->PerformKsmps();
                    std::cout << "Performing KSMPS" << std::endl;
                    ksmpsIndex = 0;
                }

                for (int chans = 0; chans < channels; chans++)
                {
                    position = ksmpsIndex * channels;
                    csoundInput[chans + position] = *inbuffer++;
                    *outbuffer++ = csoundOutput[chans + position];
                }

                ksmpsIndex++;
            }

        }
    }

    int CsoundUnity::CompileCsound(std::string csdFile)
    {
        csoundInitialize(CSOUNDINIT_NO_ATEXIT);
        std::cout << "================= Compiling Csound =====================" << std::endl;
        std::cout << "Csound file: " + csdFile;

        csound = std::make_unique<Csound>();
        csound->SetHostImplementedMIDIIO(true);
        csound->SetHostImplementedAudioIO(1, 0);
        csound->SetHostData(this);
        csound->SetOption((char*)"-n");
        csound->SetOption((char*)"-d");
        csound->SetOption((char*)"-b0");

        csoundReturnCode = csound->Compile(csdFile.c_str());

        if (csoundReturnCode == 0)
        {
            std::cout << "Successfully compiled Csound file..";
            ksmps = csound->GetKsmps();
            cs_scale = csound->Get0dBFS();
            csoundInput = csound->GetSpin();
            csoundOutput = csound->GetSpout();
        }
        else
        {
            std::cout << "Unable to compile Csound file..";
        }

        return csoundCompileOk();

    }


#ifdef WIN32
    std::string GetCsdFilename()
    {
        char   DllPath[MAX_PATH] = { 0 };
#ifdef WIN32
        GetModuleFileName((HINSTANCE)&__ImageBase, DllPath, _countof(DllPath));
        std::string fileName = DllPath;
        size_t lastindex = fileName.find_last_of(".");
        std::string fullFilename = fileName.substr(0, lastindex);
        fullFilename.append(".csd");
        return fullFilename;
#endif

    }
#else
    std::string GetCsdFilename(void)
    {
        Dl_info info;
        if (dladdr((void*)"GetCsdFilename", &info))
        {
            std::string fileName = info.dli_fname;
            size_t lastindex = fileName.find_last_of(".");
            std::string fullFilename = fileName.substr(0, lastindex);
            fullFilename.append(".csd");
            return fullFilename;
        }
    }
#endif

    //===========================================================
    // to remove leading and trailing spaces from strings....
    //===========================================================
    std::string Trim(std::string s)
    {
        //trim spaces at start
        s.erase(0, s.find_first_not_of(" \t\n"));
        //trim spaces at end
        s.erase(s.find_last_not_of(" \t\n") + 1);
        return s;
    }

    //================================================================
    // simple function for loading information about controls 
    // and Csound channels to vector
    //================================================================
    static std::vector<CsoundChannel> GetCsoundChannelVector(std::string csdFile)
    {
        std::vector<CsoundChannel> csndChannels;

        std::ifstream input(csdFile.c_str());

        std::string line;
        while (std::getline(input, line))
        {
            if (line.find("</") != std::string::npos)
                break;

            std::string newLine = line;
            std::string control = line.substr(0, line.find(" ") != std::string::npos ? line.find(" ") : 0);
            std::string::size_type i = newLine.find(control);

            if (i != std::string::npos)
                newLine.erase(i, control.length());

            if (control.find("slider") != std::string::npos ||
                control.find("button") != std::string::npos ||
                control.find("checkbox") != std::string::npos ||
                control.find("form") != std::string::npos)
            {
                CsoundChannel csndChannel;
                csndChannel.type = control;
                //init range
                csndChannel.range[MIN] = 0;
                csndChannel.range[MAX] = 1;
                csndChannel.range[VALUE] = 0;

                if (line.find("debug") != std::string::npos)
                {
                    debugMode = true;
                }

                if (line.find("caption(") != std::string::npos)
                {
                    std::string infoText = line.substr(line.find("caption(") + 9);
                    infoText = infoText.substr(0, infoText.find(")") - 1);
                    csndChannel.caption = infoText;
                }

                if (line.find("text(") != std::string::npos)
                {
                    std::string text = line.substr(line.find("text(") + 6);
                    text = text.substr(0, text.find(")") - 1);
                    csndChannel.text = text;
                }

                if (line.find("channel(") != std::string::npos)
                {
                    std::string channel = line.substr(line.find("channel(") + 9);
                    channel = channel.substr(0, channel.find(")") - 1);
                    csndChannel.name = channel;
                }

                if (line.find("range(") != std::string::npos)
                {
                    std::string range = line.substr(line.find("range(") + 6);
                    range = range.substr(0, range.find(")"));
                    char* p = strtok(&range[0u], ",");
                    int argCount = 0;
                    while (p)
                    {
                        csndChannel.range[argCount] = static_cast<float>(atof(p));
                        argCount++;
                        //not handling increment or log sliders yet
                        if (argCount == 3)
                            break;
                        p = strtok(NULL, ",");
                    }
                }

                if (line.find("value(") != std::string::npos)
                {
                    std::string value = line.substr(line.find("value(") + 6);
                    value = value.substr(0, value.find(")"));
                    csndChannel.range[VALUE] = static_cast<float>(value.length() > 0 ? atof(value.c_str()) : 0);
                }
                csndChannels.push_back(csndChannel);
            }
        }

        return csndChannels;
    }



    int InternalRegisterEffectDefinition(UnityAudioEffectDefinition& definition)
    {
        csdFilename = GetCsdFilename();
        csoundChannels = GetCsoundChannelVector(csdFilename);
        numParams = csoundChannels.size();
        CsoundChannel csndChannel;

        for (int i = 0; i < numParams; i++)
        {
            if (csoundChannels[i].type == "form")
            {
                strcpy_s(definition.name, csoundChannels[i].caption.c_str());
                //remove form from control array as it does not control anything...
                csoundChannels.erase(csoundChannels.begin() + i);
                numParams = csoundChannels.size();
            }
        }

        numParams = csoundChannels.size();

        definition.paramdefs = new UnityAudioParameterDefinition[numParams];

        for (int i = 0; i < numParams; i++)
        {
            AudioPluginUtil::RegisterParameter(definition, csoundChannels[i].name.c_str(),
                "",
                csoundChannels[i].range[MIN],
                csoundChannels[i].range[MAX],
                csoundChannels[i].range[VALUE],
                1.0f, 3.0f, i, csoundChannels[i].text.c_str());
        }
        /*AudioPluginUtil::RegisterParameter(definition, "Attack Time", "s", 0.001f, 2.0f, 0.1f, 1.0f, 3.0f, P_ATK, "Attack time");
        AudioPluginUtil::RegisterParameter(definition, "Release Time", "s", 0.001f, 2.0f, 0.5f, 1.0f, 3.0f, P_REL, "Release time");
        AudioPluginUtil::RegisterParameter(definition, "Base Level", "%", 0.0f, 1.0f, 0.1f, 100.0f, 1.0f, P_BASE, "Base filter level");
        AudioPluginUtil::RegisterParameter(definition, "Sensitivity", "%", -1.0f, 1.0f, 0.1f, 100.0f, 1.0f, P_SENS, "Filter sensitivity");
        AudioPluginUtil::RegisterParameter(definition, "Resonance", "%", 0.0f, 1.0f, 0.1f, 100.0f, 1.0f, P_RESO, "Filter resonance");
        AudioPluginUtil::RegisterParameter(definition, "Type", "", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, P_TYPE, "Filter type (0 = lowpass, 1 = bandpass)");
        AudioPluginUtil::RegisterParameter(definition, "Depth", "", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, P_DEPTH, "Filter depth (0 = 12 dB, 1 = 24 dB)");
        AudioPluginUtil::RegisterParameter(definition, "Sidechain Mix", "%", 0.0f, 1.0f, 0.0f, 100.0f, 1.0f, P_SIDECHAIN, "Sidechain mix (0 = use input, 1 = use sidechain)");
        definition.flags |= UnityAudioEffectDefinitionFlags_IsSideChainTarget;*/
        return numParams;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(UnityAudioEffectState* state)
    {
        CsoundUnity* csound = new CsoundUnity;

        memset(csound, 0, sizeof(CsoundUnity));
        auto ret = csound->CompileCsound(GetCsdFilename());
        state->effectdata = csound;

        AudioPluginUtil::InitParametersFromDefinitions(InternalRegisterEffectDefinition, csound->data.p);

        if (ret == true)
        {
            return UNITY_AUDIODSP_OK;
        }

        return UNITY_AUDIODSP_ERR_UNSUPPORTED;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback(UnityAudioEffectState* state)
    {
        CsoundUnity* csound = state->GetEffectData<CsoundUnity>();
        delete csound;
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback(UnityAudioEffectState* state, int index, float value)
    {
        //CsoundUnity::Data* data = &state->GetEffectData<CsoundUnity>()->data;
        CsoundUnity* csoundUnity = state->GetEffectData<CsoundUnity>();
        if (index >= numParams)
            return UNITY_AUDIODSP_ERR_UNSUPPORTED;

        if (index < csoundChannels.size())
        {
            std::string channelName = csoundChannels[index].name;
            csoundUnity->csound->SetControlChannel(csoundChannels[index].name.c_str(), value);
            return UNITY_AUDIODSP_OK;
        }

        return UNITY_AUDIODSP_ERR_UNSUPPORTED;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatParameterCallback(UnityAudioEffectState* state, int index, float* value, char *valuestr)
    {
        return UNITY_AUDIODSP_OK;
    }

    int UNITY_AUDIODSP_CALLBACK GetFloatBufferCallback(UnityAudioEffectState* state, const char* name, float* buffer, int numsamples)
    {
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(UnityAudioEffectState* state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int outchannels)
    {
        CsoundUnity* csoundUnity = state->GetEffectData<CsoundUnity>();
        csoundUnity->process(outbuffer, inbuffer, length, outchannels);      

        return UNITY_AUDIODSP_OK;
    }
}
