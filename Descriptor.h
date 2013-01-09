/*
	Descriptor.h
	
	Copyright 2004-11 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Creating a LADSPA_Descriptor for a CAPS plugin via a C++ template,
	saving a virtual function call compared to the usual method used
	for C++ plugins in a C context.

	Descriptor<P> expects P to declare some common methods, like init(),
	activate() etc, plus a static port_info[] and LADSPA_Data * ports[]
	and adding_gain.  (P should derive from Plugin, too.)
 
*/
/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 3
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA or point your web browser to http://www.gnu.org.
*/

#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_

#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE3__
#include <pmmintrin.h>
#endif

inline void
processor_specific_denormal_measures()
{
	#ifdef __SSE3__
	/* this one works reliably on a 6600 Core2 */
	_MM_SET_DENORMALS_ZERO_MODE (_MM_DENORMALS_ZERO_ON);
	#endif

	#ifdef __SSE__
	/* this one doesn't ... */
	_MM_SET_FLUSH_ZERO_MODE (_MM_FLUSH_ZERO_ON);
	#endif
}

template <class T>
class Descriptor
: public LV2_Descriptor
{
	public:
		Descriptor (const char *uri) {
            URI = uri;
        }

		/* in plugin's .cc file because it needs port_info implementation */
		void setup(); 
       
        const char *Label;
        const char *Maker; 
        const char *Name; 
        const char *Copyright; 

        void autogen() {return; };

		static LV2_Handle _instantiate (
                const LV2_Descriptor *descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
			{ 
				T * plugin = new T();
				int n = (int) (sizeof (T::port_info) / sizeof (PortInfo)); 
				plugin->ports = new sample_t * [n];

				plugin->fs = sample_rate;
				plugin->normal = NOISE_FLOOR;
				plugin->init();

				return plugin;
			}
		
		static void _connect_port (LV2_Handle h, uint32_t i, void * p)
			{ 
				((T *) h)->ports[i] = p;
			}

		static void _activate (LV2_Handle h)
			{
				T * plugin = (T *) h;

				plugin->first_run = 1;

				/* since none of the plugins do any RT-critical work in 
				 * activate(), it's safe to defer the actual call into
				 * the first run() after the host called activate().
				 * 
				 * It's the simplest way to prevent a parameter smoothing sweep
				 * in the first audio block after activation.
				plugin->activate();
				 */
			}

		static void _run (LV2_Handle h, ulong n)
			{
				T * plugin = (T *) h;

				/* We don't reset the processor flags later, it's true. */
				processor_specific_denormal_measures();

				/* If this is the first audio block after activation, 
				 * initialize the plugin from the current set of parameters. */
				if (plugin->first_run)
				{
					plugin->activate();
					plugin->first_run = 0;
				}

				plugin->run (n);
				plugin->normal = -plugin->normal;
			}
		
		static void _run_adding (LV2_Handle h, ulong n)
			{
				T * plugin = (T *) h;

				/* We don't reset the processor flags later, it's true. */
				processor_specific_denormal_measures();

				/* If this is the first audio block after activation, 
				 * initialize the plugin from the current set of parameters. */
				if (plugin->first_run)
				{
					plugin->activate();
					plugin->first_run = 0;
				}

				plugin->run_adding (n);
				plugin->normal = -plugin->normal;
			}
		
		static void _set_run_adding_gain (LV2_Handle h, LADSPA_Data g)
			{
				T * plugin = (T *) h;

				plugin->adding_gain = g;
			}

		static void _cleanup (LV2_Handle h)
			{
				T * plugin = (T *) h;

				delete [] plugin->ports;
				delete plugin;
			}
};

#endif /* _DESCRIPTOR_H_ */