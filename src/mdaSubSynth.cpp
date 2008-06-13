#include "mdaSubSynth.h"

#include <math.h>
#include <stdio.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaSubSynth(audioMaster);
}

mdaSubSynth::mdaSubSynth(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster, 1, 6)	// programs, parameters
{
  //inits here!
  fParam1 = (float)0.0; //type
  fParam2 = (float)0.3; //level
  fParam3 = (float)0.6; //tune
  fParam4 = (float)1.0; //dry mix 
  fParam5 = (float)0.6; //thresh
  fParam6 = (float)0.65; //release

  setNumInputs(2);		  
	setNumOutputs(2);		  
	setUniqueID('mdaB');    // identify here
	DECLARE_VST_DEPRECATED(canMono) ();				      
	canProcessReplacing();	
	strcpy(programName, "Sub Bass Synthesizer");

  resume();
}

mdaSubSynth::~mdaSubSynth()
{
	
}

bool  mdaSubSynth::getProductString(char* text) { strcpy(text, "mda SubSynth"); return true; }
bool  mdaSubSynth::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaSubSynth::getEffectName(char* name)    { strcpy(name, "SubSynth"); return true; }

void mdaSubSynth::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaSubSynth::getProgramName(char *name)
{
	strcpy(name, programName);
}

void mdaSubSynth::setProgram(VstInt32 program)
{
}

void mdaSubSynth::setParameter(VstInt32 index, float value)
{
	switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam3 = value; break;
    case 3: fParam4 = value; break;
    case 4: fParam5 = value; break;
    case 5: fParam6 = value; break;
  }

  dvd = 1.0;
  phs = 1.0;
  osc = 0.0; //oscillator phase
  typ = int(3.5 * fParam1);
  filti = (typ == 3)? 0.018f : (float)pow(10.0,-3.0 + (2.0 * fParam3));
  filto = 1.0f - filti;
  wet = fParam2;
  dry = fParam4;
  thr = (float)pow(10.0,-3.0 + (3.0 * fParam5));
  rls = (float)(1.0 - pow(10.0, -2.0 - (3.0 * fParam6)));
  dphi = (float)(0.456159 * pow(10.0,-2.5 + (1.5 * fParam3)));
}

float mdaSubSynth::getParameter(VstInt32 index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam2; break;
    case 2: v = fParam3; break;
    case 3: v = fParam4; break;
    case 4: v = fParam5; break;
    case 5: v = fParam6; break;
  }
  return v;
}

void mdaSubSynth::getParameterName(VstInt32 index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Type"); break;
    case 1: strcpy(label, "Level"); break;
    case 2: strcpy(label, "Tune"); break;
    case 3: strcpy(label, "Dry Mix"); break;
    case 4: strcpy(label, "Thresh"); break;
    case 5: strcpy(label, "Release"); break;
  }
}

void mdaSubSynth::getParameterDisplay(VstInt32 index, char *text)
{
 	char string[16];

	switch(index)
  {
    case 1: sprintf(string, "%ld", (long)(100.0f * wet)); break;
    case 2: sprintf(string, "%ld", (long)(0.0726 * getSampleRate() * pow(10.0,-2.5 + (1.5 * fParam3)))); break;
    case 3: sprintf(string, "%ld", (long)(100. * dry)); break;
    case 4: sprintf(string, "%.1f", 60.0f * fParam5 - 60.0f); break;
    case 5: sprintf(string, "%ld", (long)(-301.03 / (getSampleRate() * log10(rls)))); break;
    
    case 0: switch(typ)
    {
      case 0: strcpy(string, "Distort"); break;
      case 1: strcpy(string, "Divide"); break;
      case 2: strcpy(string, "Invert"); break;
      case 3: strcpy(string, "Key Osc."); break;
    }
  }

  string[8] = 0;
	strcpy(text, (char *)string);
}

void mdaSubSynth::getParameterLabel(VstInt32 index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, " " ); break;
    case 1: strcpy(label, "% "); break;
    case 2: strcpy(label, "Hz"); break; 
    case 3: strcpy(label, "%" ); break; 
    case 4: strcpy(label, "dB"); break; 
    case 5: strcpy(label, "ms"); break; 
  }
}


void mdaSubSynth::resume()
{
  phi = env = filt1 = filt2 = filt3 = filt4 = filti = filto = 0.0f;

  setParameter(0, getParameter(0));
}

//--------------------------------------------------------------------------------
// process

void mdaSubSynth::process(float **inputs, float **outputs, VstInt32 sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d;	
  float we, dr, fi, fo, f1, f2, f3, f4, sub, rl, th, dv, ph, phii, dph, os, en;

  dph = dphi;
  rl = rls;
  phii = phi;
  en = env;
  os = osc;
  th = thr;
  dv = dvd;
  ph = phs;
  we = wet;
  dr = dry;
  f1 = filt1;
  f2 = filt2;
  f3 = filt3;
  f4 = filt4;
  fi = filti;
  fo = filto;
    
  --in1;	
	--in2;	
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;		
		b = *++in2;
		
    c = out1[1];
		d = out2[1]; //process from here...

    f1 = (fo * f1) + (fi * (a + b));
    f2 = (fo * f2) + (fi * f1);

    sub = f2;
    if (sub > th)
    {
      sub = 1.0;        
    }
    else
    {
      if(sub < -th)
      {
        sub = -1.0;
      }
      else
      {
        sub = 0.0;
      }
    }
    
    if((sub * dv) < 0) //octave divider
    {
      dv = -dv; if(dv < 0.) ph = -ph;
    }

    if(typ == 1) //divide
    {
      sub = ph * sub;
    }
    if(typ == 2) //invert
    {
      sub = (float)(ph * f2 * 2.0);
    }
    if(typ == 3) //osc
    {
      if (f2 > th) {en = 1.0; } 
      else {en = en * rl;}
      sub = (float)(en * sin(phii));
      phii = (float)fmod( phii + dph, 6.283185f );
    }
    
    f3 = (fo * f3) + (fi * sub);
    f4 = (fo * f4) + (fi * f3);

		c += (a * dr) + (f4 * we); // output
		d += (b * dr) + (f4 * we);
		    
    *++out1 = c;	
		*++out2 = d;
	}
  if(fabs(f1)<1.0e-10) filt1=0.f; else filt1=f1;
  if(fabs(f2)<1.0e-10) filt2=0.f; else filt2=f2;
  if(fabs(f3)<1.0e-10) filt3=0.f; else filt3=f3;
  if(fabs(f4)<1.0e-10) filt4=0.f; else filt4=f4;
  dvd = dv;
  phs = ph;
  osc = os;
  phi = phii;
  env = en;
}

void mdaSubSynth::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d;	
  float we, dr, fi, fo, f1, f2, f3, f4, sub, rl, th, dv, ph, phii, dph, os, en;

  dph = dphi;
  rl = rls;
  phii = phi;
  en = env;
  os = osc;
  th = thr;
  dv = dvd;
  ph = phs;
  we = wet;
  dr = dry;
  f1 = filt1;
  f2 = filt2;
  f3 = filt3;
  f4 = filt4;

  fi = filti;
  fo = filto;
 
  --in1;	
	--in2;	
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;		
		b = *++in2; //process from here...
		
    f1 = (fo * f1) + (fi * (a + b));
    f2 = (fo * f2) + (fi * f1);

    sub = f2;
    if (sub > th)
    {
      sub = 1.0;        
    }
    else
    {
      if(sub < -th)
      {
        sub = -1.0;
      }
      else
      {
        sub = 0.0;
      }
    }
    
    if((sub * dv) < 0) //octave divider
    {
      dv = -dv; if(dv < 0.) ph = -ph;
    }

    if(typ == 1) //divide
    {
      sub = ph * sub;
    }
    if(typ == 2) //invert
    {
      sub = (float)(ph * f2 * 2.0);
    }
    if(typ == 3) //osc
    {
      if (f2 > th) {en = 1.0; } 
      else {en = en * rl;}
      sub = (float)(en * sin(phii));
      phii = (float)fmod( phii + dph, 6.283185f );
    }
    
    f3 = (fo * f3) + (fi * sub);
    f4 = (fo * f4) + (fi * f3);

		c = (a * dr) + (f4 * we); // output
		d = (b * dr) + (f4 * we);

    *++out1 = c;
		*++out2 = d;
	}
  if(fabs(f1)<1.0e-10) filt1=0.f; else filt1=f1;
  if(fabs(f2)<1.0e-10) filt2=0.f; else filt2=f2;
  if(fabs(f3)<1.0e-10) filt3=0.f; else filt3=f3;
  if(fabs(f4)<1.0e-10) filt4=0.f; else filt4=f4;
  dvd = dv;
  phs = ph;
  osc = os;
  phi = phii;
  env = en;
}
