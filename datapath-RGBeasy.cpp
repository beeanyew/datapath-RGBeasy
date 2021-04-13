#include <obs-module.h>
#include <obs-source.h>
#include <obs.h>
#include <util/platform.h>
#include <Windows.h>

#include <API.H>
#include <RGB.H>
#include <RGBAPI.H>

#include "datapath-RGBeasy.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("datapath-RGBeasy", "en-US")

HRGBDLL rgbdll = 0;
RGBINPUTINFO inputinfo;

uint16_t num_inputs = 0;
uint16_t cur_texture = 0;

bool obs_module_load()
{
	obs_source_info rgbeasy_info = {};
	rgbeasy_info.id = "datapath_rgbeasy_source";
	rgbeasy_info.type = OBS_SOURCE_TYPE_INPUT;
	rgbeasy_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC;
	rgbeasy_info.get_name = rgbeasy_getname;
	rgbeasy_info.create = rgbeasy_create;
	rgbeasy_info.destroy = rgbeasy_destroy;
	rgbeasy_info.update = rgbeasy_update;
	rgbeasy_info.get_width = rgbeasy_getwidth;
	rgbeasy_info.get_height = rgbeasy_getheight;
	rgbeasy_info.show = rgbeasy_show;
	rgbeasy_info.hide = rgbeasy_hide;
	//rgbeasy_info.video_render = rgbeasy_render;
	rgbeasy_info.get_properties = rgbeasy_properties;
	rgbeasy_info.get_defaults = rgbeasy_defaults;

	unsigned long error = 0;
	unsigned char cur_input = 0;
	signed long livestream_available = 0;

	error = RGBLoad(&rgbdll);
	if (error) {
		blog(LOG_WARNING, "Failed to load RGBEasy DLL.");
		return false;
	}
	blog(LOG_INFO, "RGBEasy DLL successfully loaded.\nEnumerating inputs...");

	inputinfo.Size = sizeof(inputinfo);
	error = RGBGetInputInfo(cur_input, &inputinfo);

	blog(LOG_INFO, "Driver version: %i.%i.%i rev%i", inputinfo.Driver.Major,
		inputinfo.Driver.Minor, inputinfo.Driver.Micro, inputinfo.Driver.Revision);

	while (!error) {
		if (!error) {
			num_inputs++;
			blog(LOG_INFO, "Input %i:", cur_input + 1);
			blog(LOG_INFO, "Chassis: %i (%i)", inputinfo.Chassis.Slot, inputinfo.Chassis.Index);
			blog(LOG_INFO, "Firmware: %.8X", inputinfo.FirmWare);
			blog(LOG_INFO, "VHDL: %.8X", inputinfo.VHDL);
			blog(LOG_INFO, "Identifier: %.8X", inputinfo.Identifier);
			RGBInputIsLiveStreamSupported(cur_input, &livestream_available);
			blog(LOG_INFO, "LiveStream supported: %s", (livestream_available) ? "Yes" : "No");
		}
		cur_input++;
		error = RGBGetInputInfo(cur_input, &inputinfo);
	}

	obs_register_source(&rgbeasy_info);

	return true;
}

void obs_module_unload(void)
{
	if (rgbdll != 0) {
		RGBFree(rgbdll);
	}
}

static const char *rgbeasy_getname(void* type_data)
{
	return obs_module_text("RGBEasy Video Capture");
}

static uint32_t rgbeasy_getwidth(void *data)
{
	struct rgbeasy_src *srcdata = (rgbeasy_src *)data;

	//return 640;
	return srcdata->cx;
}

static uint32_t rgbeasy_getheight(void *data)
{
	struct rgbeasy_src *srcdata = (rgbeasy_src*)data;

	//return 480;
	return srcdata->cy;
}

static obs_properties_t *rgbeasy_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(props, "cur_input",
		obs_module_text("Input"), 1, num_inputs, 1);

	obs_properties_add_int(props, "output_width",
		obs_module_text("Output width"), 64, 8192, 1);

	obs_properties_add_int(props, "output_height",
		obs_module_text("Output height"), 64, 1200, 1);

	obs_properties_add_int(props, "capture_width",
		obs_module_text("Horizontal Size"), 64, 8192, 1);

	obs_properties_add_int(props, "position_x",
		obs_module_text("X Position"), 0, 8192, 1);

	obs_properties_add_int(props, "position_y",
		obs_module_text("Y Position"), 0, 1200, 1);

	obs_properties_add_int(props, "capture_phase",
		obs_module_text("Phase"), 0, 32, 1);

	obs_properties_add_int(props, "black_level",
		obs_module_text("Black Level"), 0, 255, 1);

	obs_properties_add_int(props, "brightness",
		obs_module_text("Brightness"), 0, 127, 1);

	obs_properties_add_int(props, "contrast",
		obs_module_text("Contrast"), 0, 255, 1);

	obs_properties_add_int(props, "brightness_r",
		obs_module_text("Brightness (Red)"), 0, 512, 1);

	obs_properties_add_int(props, "brightness_g",
		obs_module_text("Brightness (Green)"), 0, 512, 1);

	obs_properties_add_int(props, "brightness_b",
		obs_module_text("Brightness (Blue)"), 0, 512, 1);

	obs_properties_add_int(props, "contrast_r",
		obs_module_text("Contrast (Red)"), 0, 512, 1);

	obs_properties_add_int(props, "contrast_g",
		obs_module_text("Contrast (Green)"), 0, 512, 1);

	obs_properties_add_int(props, "contrast_b",
		obs_module_text("Contrast (Blue)"), 0, 512, 1);

	obs_properties_add_int(props, "contrast_b",
		obs_module_text("Color Space"), 0, RGB_COLOURDOMAINDETECT_AUTO, 1);

	obs_properties_add_int(props, "num_nibbles",
		obs_module_text("Number of nibbles"), 0, 14, 1);

	return props;
}

void set_capture_defaults(struct rgbeasy_src *srcdata) {
	if (srcdata->hrgb == (HRGB)NULL || srcdata->settings == NULL) {
		blog(LOG_WARNING, "WARNUNG: Something is null.");
		return;
	}

	unsigned long width = 0, height = 0;
	long x = 0, y = 0;

	signed long phase;

	//RGBGetOutputSize(srcdata->hrgb, &width, &height);
	RGBGetCaptureWidth(srcdata->hrgb, &width);
	RGBGetCaptureHeight(srcdata->hrgb, &height);
	obs_data_set_int(srcdata->settings, "output_width", width);
	obs_data_set_int(srcdata->settings, "output_height", height);
	blog(LOG_INFO, "Output size: %ix%i", width, height);
	RGBGetHorScale(srcdata->hrgb, &width);
	obs_data_set_int(srcdata->settings, "capture_width", width);
	blog(LOG_INFO, "Capture size: %ix%i", width, height);
	RGBGetHorPosition(srcdata->hrgb, &x);
	RGBGetVerPosition(srcdata->hrgb, &y);
	obs_data_set_int(srcdata->settings, "position_x", x);
	obs_data_set_int(srcdata->settings, "position_y", y);
	RGBGetPhase(srcdata->hrgb, &phase);
	RGBGetBlackLevel(srcdata->hrgb, &y);
	obs_data_set_int(srcdata->settings, "capture_phase", phase);
	obs_data_set_int(srcdata->settings, "black_level", y);
	RGBGetBrightness(srcdata->hrgb, &x);
	RGBGetContrast(srcdata->hrgb, &y);
	obs_data_set_int(srcdata->settings, "brightness", x);
	obs_data_set_int(srcdata->settings, "contrast", y);
}

static void rgbeasy_defaults(obs_data_t* settings)
{
	obs_data_set_default_int(settings, "cur_input", 1);
	obs_data_set_default_int(settings, "output_width", 320);
	obs_data_set_default_int(settings, "output_height", 240);
	obs_data_set_default_int(settings, "capture_width", 341);
	obs_data_set_default_int(settings, "position_x", 100);
	obs_data_set_default_int(settings, "position_y", 8);
	obs_data_set_default_int(settings, "capture_phase", 0);
	obs_data_set_default_int(settings, "black_level", 8);
	obs_data_set_default_int(settings, "brightness", 32);
	obs_data_set_default_int(settings, "brightness_r", 128);
	obs_data_set_default_int(settings, "brightness_g", 128);
	obs_data_set_default_int(settings, "brightness_b", 128);
	obs_data_set_default_int(settings, "contrast", 128);
	obs_data_set_default_int(settings, "contrast_r", 256);
	obs_data_set_default_int(settings, "contrast_g", 256);
	obs_data_set_default_int(settings, "contrast_b", 256);
	obs_data_set_default_int(settings, "num_nibbles", 1);
}

#define FORCE(a) ret = 55; force_loops = 0; while(force_loops < 3000 &&ret != 0 && ret != RGB_ERROR_INVALIDDATA && ret != RGB_ERROR_UNKNOWN) { ret = a; }
#define CHKERR(a) if (force_loops == 3000) { blog(LOG_WARNING, "Error code %8X attempting to apply RGBeasy setting %d.", ret, a); }

void reset_capture_settings(struct rgbeasy_src *srcdata) {
	if (srcdata->hrgb == (HRGB)NULL || srcdata->settings == NULL) return;
	unsigned long ret = 55;
	unsigned int force_loops = 0;

	if (obs_data_get_int(srcdata->settings, "capture_width") != 0 && obs_data_get_int(srcdata->settings, "output_width") != 0 && obs_data_get_int(srcdata->settings, "output_height") != 0) {
		FORCE(RGBSetOutputSize(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "output_width"), (unsigned long)obs_data_get_int(srcdata->settings, "output_height")));
		FORCE(RGBSetCaptureWidth(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "output_width")));
		FORCE(RGBSetCaptureHeight(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "output_height")));
		FORCE(RGBSetHorPosition(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "position_x")));
		FORCE(RGBSetVerPosition(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "position_y")));
		FORCE(RGBSetPhase(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "capture_phase")));
		FORCE(RGBSetBlackLevel(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "black_level")));
		FORCE(RGBSetBrightness(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "brightness")));
		FORCE(RGBSetContrast(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "contrast")));
		FORCE(RGBSetHorScale(srcdata->hrgb, (unsigned long)obs_data_get_int(srcdata->settings, "capture_width")));
		uint32_t b_r, b_g, b_b, c_r, c_g, c_b;
		b_r = (unsigned long)obs_data_get_int(srcdata->settings, "brightness_r");
		b_g = (unsigned long)obs_data_get_int(srcdata->settings, "brightness_g");
		b_b = (unsigned long)obs_data_get_int(srcdata->settings, "brightness_b");
		c_r = (unsigned long)obs_data_get_int(srcdata->settings, "contrast_r");
		c_g = (unsigned long)obs_data_get_int(srcdata->settings, "contrast_g");
		c_b = (unsigned long)obs_data_get_int(srcdata->settings, "contrast_b");
		RGBSetColourBalance(srcdata->hrgb, b_r, b_g, b_b, c_r, c_g, c_b);
		RGBSetColourDomain(srcdata->hrgb, (COLOURDOMAINDETECT)obs_data_get_int(srcdata->settings, "color_space"));
	}

	return;
}

static void rgbeasy_show(void* data) {
	struct rgbeasy_src* srcdata = (rgbeasy_src *)data;
	if (srcdata->capture_active)
		return;
	blog(LOG_INFO, "Called rgbeasy_show for source %s", obs_source_get_name(srcdata->src));

	/*RGBSetFrameCapturedFn(srcdata->hrgb, getframe, (ULONG_PTR)srcdata);
	blog(LOG_INFO, "Show: Set FrameCapturedFn");
	RGBSetNoSignalFn(srcdata->hrgb, nosignal, (ULONG_PTR)srcdata);
	blog(LOG_INFO, "Show: Set NoSignalFn");
	RGBSetValueChangedFn(srcdata->hrgb, valuechanged, (ULONG_PTR)srcdata);
	blog(LOG_INFO, "Show: Set ValueChangedFn");
	RGBSetWindow(srcdata->hrgb, NULL);
	blog(LOG_INFO, "Show: Set Window");*/
	rgbeasy_update(data, srcdata->settings);
	//RGBStartCapture(srcdata->hrgb);
	blog(LOG_INFO, "Show: Started Capture");
	srcdata->capture_active = true;
}

static void rgbeasy_hide(void* data) {
	struct rgbeasy_src* srcdata = (rgbeasy_src*)data;
	if (srcdata->hrgb == 0)
		return;
	blog(LOG_INFO, "Called rgbeasy_hide for source %s", obs_source_get_name(srcdata->src));

	srcdata->capture_active = false;
	RGBUseOutputBuffers(srcdata->hrgb, FALSE);
	while (srcdata->grabbing_frame) { os_sleep_ms(0); }
	while (RGBStopCapture(srcdata->hrgb) != RGBINPUT_NOERROR) {}
	RGBCloseInput(srcdata->hrgb);
	RGBSetFrameCapturedFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
	RGBSetValueChangedFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
	RGBSetNoSignalFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
	srcdata->hrgb = 0;
	blog(LOG_INFO, "Hide: Stopped capture");
	/*RGBSetFrameCapturedFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
	blog(LOG_INFO, "Hide: Set FrameCapturedFn");
	RGBSetNoSignalFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
	blog(LOG_INFO, "Hide: Set NoSignalFn");
	RGBSetValueChangedFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
	blog(LOG_INFO, "Hide: Set ValueChangedFn");*/
}
#undef FORCE
#undef CHKERR

static void rgbeasy_destroy(void *data)
{
	struct rgbeasy_src *srcdata = (rgbeasy_src*)data;

	if (srcdata->hrgb != 0) {
		RGBSetFrameCapturedFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
		RGBSetValueChangedFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
		RGBUseOutputBuffers(srcdata->hrgb, FALSE);
		while (srcdata->grabbing_frame) { os_sleep_ms(0); }
		RGBStopCapture(srcdata->hrgb);
		RGBCloseInput(srcdata->hrgb);
		srcdata->hrgb = 0;
	}

	obs_source_output_video(srcdata->src, NULL);

	bfree(srcdata);
}

void set_frame_crap(struct obs_source_frame2 *frm, int width, int height) {
	frm->color_range_max[0] =
		frm->color_range_max[1] =
		frm->color_range_max[2] = 1.0f;
	frm->color_range_min[0] =
		frm->color_range_min[1] =
		frm->color_range_min[2] = 0.0f;
	frm->format = VIDEO_FORMAT_BGRX;
	frm->range = VIDEO_RANGE_FULL;
	frm->width = width;
	frm->height = height;
	frm->linesize[0] = width * 4;
}

static void *rgbeasy_create(obs_data_t *settings, obs_source_t *source)
{
	struct rgbeasy_src *srcdata = (rgbeasy_src*)bzalloc(sizeof(struct rgbeasy_src));
	
	srcdata->src = source;
	srcdata->cur_input = 1;

	srcdata->cx = 640;
	srcdata->cy = 480;
	srcdata->newlycreated = true;
	srcdata->capture_active = false;

	set_frame_crap(&srcdata->video_frame, 640, 480);

	rgbeasy_update(srcdata, settings);

	uint32_t flags = obs_source_get_flags(source);
	obs_source_set_async_unbuffered(source, true);

	srcdata->lastframe = os_gettime_ns();

	return srcdata;
}

static void rgbeasy_update(void *data, obs_data_t *settings)
{
	struct rgbeasy_src *srcdata = (rgbeasy_src*)data;
	bool restart_input = false;

	if (settings == NULL) return;

	srcdata->settings = settings;

	blog(LOG_INFO, "Called rgbeasy_update on %s.", obs_source_get_name(srcdata->src));

	unsigned long result = 0;

	uint8_t set_input = (uint8_t)obs_data_get_int(settings, "cur_input");

	if (set_input != srcdata->cur_input) {
		if (set_input == 0) {
			set_input = 1;
		}
		srcdata->cur_input = set_input;
		obs_data_set_int(settings, "cur_input", srcdata->cur_input);
		restart_input = true;
	}

	if (restart_input || srcdata->newlycreated || !srcdata->capture_active) {
		if (srcdata->hrgb != 0) {
			srcdata->capture_active = false;
			RGBSetFrameCapturedFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
			RGBStopCapture(srcdata->hrgb);
			RGBCloseInput(srcdata->hrgb);
			srcdata->hrgb = 0;
		}

		result = RGBOpenInput(srcdata->cur_input - 1, &srcdata->hrgb);
		if (result != 0) {
			blog(LOG_WARNING, "Failed to open RGB input #%i.", srcdata->cur_input);
			goto capture_setup_error;
		}
		blog(LOG_INFO, "RGB Input #%i opened successfully.", srcdata->cur_input);

		RGBSetDMADirect(srcdata->hrgb, 1);
		RGBSetPixelFormat(srcdata->hrgb, RGB_PIXELFORMAT_RGB24);
		RGBSetFrameDropping(srcdata->hrgb, 0);
		if (RGBSetLiveStream(srcdata->hrgb, LIVESTREAM_1) != 0) {
			blog(LOG_INFO, "Failed to enable LiveStream for input %d", srcdata->cur_input);
		}
	
		RGBSetFrameCapturedFn(srcdata->hrgb, getframe, (ULONG_PTR)srcdata);
		RGBSetNoSignalFn(srcdata->hrgb, nosignal, (ULONG_PTR)srcdata);
		RGBSetValueChangedFn(srcdata->hrgb, valuechanged, (ULONG_PTR)srcdata);
		RGBSetWindow(srcdata->hrgb, NULL);
		RGBStartCapture(srcdata->hrgb);
		srcdata->capture_active = true;
		srcdata->first_frame = 1;

		set_capture_defaults(srcdata);
		//blog(LOG_INFO, "We're past the capture defaults.");
		obs_source_update_properties(srcdata->src);
		//blog(LOG_INFO, "Properties updated.");

		//RGBGetOutputSize(srcdata->hrgb, &srcdata->cx, &srcdata->cy);
	}

	if (srcdata->newlycreated) srcdata->newlycreated = false;

	reset_capture_settings(srcdata);
	return;

capture_setup_error:;
	printf("Something went wrong with the capture setup.");
}

static void rgbeasy_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(effect);
}

void RGBCBKAPI getframe(HWND hWnd, HRGB hRGB, LPBITMAPINFOHEADER bitinfo, void *pBitmapBits, ULONG_PTR userData) {
	struct rgbeasy_src *srcdata = (struct rgbeasy_src *)userData;

	//blog(LOG_INFO, "We're here");

	uint64_t start = 0, end = 0;
	size_t i = 0;

	float time1, time2;

	if (hRGB == 0 || !bitinfo || !pBitmapBits || !userData || !srcdata->capture_active || !srcdata->hrgb)
		return;

	if (srcdata->first_frame) {
		srcdata->first_frame = 0;
		return;
	}

	start = os_gettime_ns();

	end = start - srcdata->lastframe;
	time1 = (float)end / 1000000.0f;
	time2 = 1000.0f / time1;

	srcdata->lastframe = start;	

	unsigned long width, height;

	if (bitinfo->biWidth == 0 || -(bitinfo->biHeight) == 0)
		return;

	width = bitinfo->biWidth;
	height = -(bitinfo->biHeight);
	set_frame_crap(&srcdata->video_frame, width, height);
	srcdata->cx = width;
	srcdata->cy = height;
	obs_data_set_int(srcdata->settings, "output_width", width);
	obs_data_set_int(srcdata->settings, "output_height", height);

	if (width != srcdata->cx || height != srcdata->cy)
		blog(LOG_INFO, "Allocating %lld x %lld * 4 bytes (%lld)", width, height, width * height * 4);
	unsigned int *tmpbuf = (unsigned int *)bzalloc((size_t)width * (size_t)height * 4);
	if (!tmpbuf)
		return;

	if (hRGB == 0 || !bitinfo || !pBitmapBits || !userData || !srcdata->capture_active || !srcdata->hrgb)
		return;

	os_sleep_ms(0);
	if (!srcdata->capture_active)
		goto race_failure;

	while (!srcdata->grabbing_frame)
		srcdata->grabbing_frame = true;
	for (i = 0; i + 16 < (uint64_t)width * ((uint64_t)height - 1); i += 16) {
		memcpy(&tmpbuf[i], (uint8_t*)pBitmapBits + (i * 3), 3);
		memcpy(&tmpbuf[i + 1], (uint8_t*)pBitmapBits + ((i + 1) * 3), 3);
		memcpy(&tmpbuf[i + 2], (uint8_t*)pBitmapBits + ((i + 2) * 3), 3);
		memcpy(&tmpbuf[i + 3], (uint8_t*)pBitmapBits + ((i + 3) * 3), 3);
		memcpy(&tmpbuf[i + 4], (uint8_t*)pBitmapBits + ((i + 4) * 3), 3);
		memcpy(&tmpbuf[i + 5], (uint8_t*)pBitmapBits + ((i + 5) * 3), 3);
		memcpy(&tmpbuf[i + 6], (uint8_t*)pBitmapBits + ((i + 6) * 3), 3);
		memcpy(&tmpbuf[i + 7], (uint8_t*)pBitmapBits + ((i + 7) * 3), 3);
		memcpy(&tmpbuf[i + 8], (uint8_t*)pBitmapBits + ((i + 8) * 3), 3);
		memcpy(&tmpbuf[i + 9], (uint8_t*)pBitmapBits + ((i + 9) * 3), 3);
		memcpy(&tmpbuf[i + 10], (uint8_t*)pBitmapBits + ((i + 10) * 3), 3);
		memcpy(&tmpbuf[i + 11], (uint8_t*)pBitmapBits + ((i + 11) * 3), 3);
		memcpy(&tmpbuf[i + 12], (uint8_t*)pBitmapBits + ((i + 12) * 3), 3);
		memcpy(&tmpbuf[i + 13], (uint8_t*)pBitmapBits + ((i + 13) * 3), 3);
		memcpy(&tmpbuf[i + 14], (uint8_t*)pBitmapBits + ((i + 14) * 3), 3);
		memcpy(&tmpbuf[i + 15], (uint8_t*)pBitmapBits + ((i + 15) * 3), 3);
	}
	while (i < (uint64_t)width * (uint64_t)height) {
		memcpy(&tmpbuf[i], (uint8_t*)pBitmapBits + (i * 3), 3);
		i++;
	}
	while(srcdata->grabbing_frame)
		srcdata->grabbing_frame = false;

	srcdata->video_frame.data[0] = (uint8_t *)tmpbuf;
	srcdata->video_frame.timestamp = (uint64_t)start;
	obs_source_output_video2(srcdata->src, &srcdata->video_frame);
race_failure:;
	if (tmpbuf)
		bfree(tmpbuf);
	return;
}

void RGBCBKAPI nosignal(HWND hWnd, HRGB hRGB, ULONG_PTR userData) {
	// No signal stuff
	struct rgbeasy_src* srcdata = (struct rgbeasy_src*)userData;

	obs_source_output_video(srcdata->src, NULL);
}

#define CHKCHANGED(a, b) if (value_info->a.BChanged) { obs_data_set_int(srcdata->settings, b, value_info->a.Value); };

void RGBCBKAPI valuechanged(HWND hWnd, HRGB hrgb, PRGBVALUECHANGEDINFO value_info, ULONG_PTR userData)
{
	struct rgbeasy_src* srcdata = (struct rgbeasy_src*)userData;

	long width = 0, height = 0;

	CHKCHANGED(CaptureWidth, "output_width");
	CHKCHANGED(CaptureHeight, "output_height");
	CHKCHANGED(HorScale, "capture_width");
	CHKCHANGED(HorPosition, "position_x");
	CHKCHANGED(VerPosition, "position_y");
	CHKCHANGED(Phase, "capture_phase");
	CHKCHANGED(BlackLevel, "black_level");
	CHKCHANGED(Brightness, "brightness");
	CHKCHANGED(Contrast, "contrast");
	CHKCHANGED(RedGain, "contrast_r");
	CHKCHANGED(GreenGain, "contrast_g");
	CHKCHANGED(BlueGain, "contrast_b");
	CHKCHANGED(RedOffset, "brightness_r");
	CHKCHANGED(GreenOffset, "brightness_g");
	CHKCHANGED(BlueOffset, "brightness_b");
	CHKCHANGED(ColourDomain, "color_space");

	rgbeasy_update((void*)srcdata, srcdata->settings);
}
