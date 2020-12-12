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

static struct obs_source_info rgbeasy_info = {
	.id = "datapath_rgbeasy_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC,
	.get_name = rgbeasy_getname,
	.create = rgbeasy_create,
	.destroy = rgbeasy_destroy,
	.update = rgbeasy_update,
	.get_width = rgbeasy_getwidth,
	.get_height = rgbeasy_getheight,
	//.video_render = rgbeasy_render,
	.get_properties = rgbeasy_properties,
	.get_defaults = rgbeasy_defaults,
};

bool obs_module_load()
{
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
	struct rgbeasy_src *srcdata = data;

	//return 640;
	return srcdata->cx;
}

static uint32_t rgbeasy_getheight(void *data)
{
	struct rgbeasy_src *srcdata = data;

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

	obs_properties_add_int(props, "num_nibbles",
		obs_module_text("Number of nibbles"), 0, 14, 1);

	return props;
}

void set_capture_defaults(struct rgbeasy_src *srcdata) {
	if (srcdata->hrgb == (HRGB)NULL || srcdata->settings == NULL) {
		blog(LOG_WARNING, "WARNUNG: Something is null.");
		return;
	}

	long width = 0, height = 0;
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
	obs_data_set_default_int(settings, "contrast", 128);
	obs_data_set_default_int(settings, "num_nibbles", 1);
}

#define FORCE(a) ret = 55; force_loops = 0; while(force_loops < 3000 &&ret != 0 && ret != RGB_ERROR_INVALIDDATA && ret != RGB_ERROR_UNKNOWN) { ret = a; }
#define CHKERR(a) if (force_loops == 3000) { blog(LOG_WARNING, "Error code %8X attempting to apply RGBeasy setting %d.", ret, a); }
void reset_capture_settings(struct rgbeasy_src *srcdata) {
	if (srcdata->hrgb == (HRGB)NULL || srcdata->settings == NULL) return;
	unsigned long ret = 55;
	unsigned int force_loops = 0;

	if (obs_data_get_int(srcdata->settings, "capture_width") != 0 && obs_data_get_int(srcdata->settings, "output_width") != 0 && obs_data_get_int(srcdata->settings, "output_height") != 0) {
		FORCE(RGBSetOutputSize(srcdata->hrgb, obs_data_get_int(srcdata->settings, "output_width"), obs_data_get_int(srcdata->settings, "output_height")));
		CHKERR(1);
		FORCE(RGBSetCaptureWidth(srcdata->hrgb, obs_data_get_int(srcdata->settings, "output_width")));
		CHKERR(2);
		FORCE(RGBSetCaptureHeight(srcdata->hrgb, obs_data_get_int(srcdata->settings, "output_height")));
		CHKERR(3);
		FORCE(RGBSetHorPosition(srcdata->hrgb, obs_data_get_int(srcdata->settings, "position_x")));
		CHKERR(4);
		FORCE(RGBSetVerPosition(srcdata->hrgb, obs_data_get_int(srcdata->settings, "position_y")));
		CHKERR(5);
		FORCE(RGBSetPhase(srcdata->hrgb, obs_data_get_int(srcdata->settings, "capture_phase")));
		CHKERR(6);
		FORCE(RGBSetBlackLevel(srcdata->hrgb, obs_data_get_int(srcdata->settings, "black_level")));
		CHKERR(7);
		FORCE(RGBSetBrightness(srcdata->hrgb, obs_data_get_int(srcdata->settings, "brightness")));
		CHKERR(8);
		FORCE(RGBSetContrast(srcdata->hrgb, obs_data_get_int(srcdata->settings, "contrast")));
		CHKERR(9);
		FORCE(RGBSetHorScale(srcdata->hrgb, obs_data_get_int(srcdata->settings, "capture_width")));
		CHKERR(10);
	}

	return;
}
#undef FORCE
#undef CHKERR

static void rgbeasy_destroy(void *data)
{
	struct rgbeasy_src *srcdata = data;

	obs_enter_graphics();

	obs_source_output_video(srcdata->src, NULL);

	if (srcdata->hrgb != 0) {
		RGBSetValueChangedFn(srcdata->hrgb, NULL, (ULONG_PTR)srcdata);
		RGBStopCapture(srcdata->hrgb);
		RGBCloseInput(srcdata->hrgb);
		srcdata->hrgb = 0;
	}

	for (uint32_t i = 0; i < 16; i++) {
		if (srcdata->textures[i] != NULL) {
			gs_texture_destroy(srcdata->textures[i]);
			srcdata->textures[i] = NULL;
		}
	}
	if (srcdata->vbuf != NULL) {
		gs_vertexbuffer_destroy(srcdata->vbuf);
		srcdata->vbuf = NULL;
	}
	if (srcdata->draw_effect != NULL) {
		gs_effect_destroy(srcdata->draw_effect);
		srcdata->draw_effect = NULL;
	}
	if (srcdata->texbuf != NULL) {
		bfree(srcdata->texbuf);
		srcdata->texbuf = NULL;
	}
	if (srcdata->cur_tex != NULL) {
		gs_texture_destroy(srcdata->cur_tex);
	}

	obs_leave_graphics();

	bfree(srcdata);
}

void set_frame_crap(struct obs_source_frame *frm, int width, int height) {
	frm->color_range_max[0] =
		frm->color_range_max[1] =
		frm->color_range_max[2] = 1.0f;
	frm->color_range_min[0] =
		frm->color_range_min[1] =
		frm->color_range_min[2] = 0.0f;
	frm->format = VIDEO_FORMAT_BGRX;
	frm->full_range = true;
	frm->width = width;
	frm->height = height;
	frm->linesize[0] = width * 4;
}

static void *rgbeasy_create(obs_data_t *settings, obs_source_t *source)
{
	struct rgbeasy_src *srcdata = bzalloc(sizeof(struct rgbeasy_src));
	
	srcdata->src = source;
	srcdata->cur_input = 1;

	srcdata->cx = 640;
	srcdata->cy = 480;
	srcdata->newlycreated = true;

	for (uint32_t i = 0; i < 16; i++) {
		if (srcdata->textures[i] != NULL) {
			srcdata->textures[i] = NULL;
		}
	}

	set_frame_crap(&srcdata->video_frame, 640, 480);
	srcdata->texbuf = NULL;

	srcdata->cur_tex = gs_texture_create(640, 480, GS_BGRX, 1, NULL, GS_DYNAMIC);

	rgbeasy_update(srcdata, settings);
	srcdata->cur_texture = 0;

	uint32_t flags = obs_source_get_flags(source);
	obs_source_set_async_unbuffered(source, true);

	srcdata->lastframe = os_gettime_ns();

	return srcdata;
}

bool capture_active = false;

static void rgbeasy_update(void *data, obs_data_t *settings)
{
	struct rgbeasy_src *srcdata = data;
	bool restart_input = false;

	if (settings == NULL) return;

	srcdata->settings = settings;

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

	if (restart_input || srcdata->newlycreated) {
		if (srcdata->hrgb != 0) {
			capture_active = false;
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
		if (RGBSetLiveStream(srcdata->hrgb, 1) != 0) {
			blog(LOG_INFO, "Failed to enable LiveStream for input %d", srcdata->cur_input);
		}
	
		RGBSetFrameCapturedFn(srcdata->hrgb, getframe, (ULONG_PTR)srcdata);
		RGBSetNoSignalFn(srcdata->hrgb, nosignal, (ULONG_PTR)srcdata);
		RGBSetValueChangedFn(srcdata->hrgb, valuechanged, (ULONG_PTR)srcdata);
		RGBStartCapture(srcdata->hrgb);
		capture_active = true;

		set_capture_defaults(srcdata);
		//blog(LOG_INFO, "We're past the capture defaults.");
		obs_source_update_properties(srcdata->src);
		//blog(LOG_INFO, "Properties updated.");


		//RGBGetOutputSize(srcdata->hrgb, &srcdata->cx, &srcdata->cy);
	}

	if (srcdata->newlycreated) srcdata->newlycreated = false;

	reset_capture_settings(srcdata);

capture_setup_error:;

	if (srcdata->draw_effect == NULL) {
		char *effect_file = NULL;
		char *error_string = NULL;

		effect_file =
			obs_module_file("default.effect");

		if (effect_file) {
			obs_enter_graphics();
			srcdata->draw_effect = gs_effect_create_from_file(
				effect_file, &error_string);
			obs_leave_graphics();

			bfree(effect_file);
			if (error_string != NULL)
				bfree(error_string);
		}
	}
}

static void rgbeasy_render(void *data, gs_effect_t *effect)
{
	struct rgbeasy_src *srcdata = data;
	if (srcdata == NULL) return;

	gs_reset_blend_state();
	if (srcdata->cur_tex != NULL) {
		gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), srcdata->cur_tex);
		gs_draw_sprite(srcdata->cur_tex, 0, srcdata->cx, srcdata->cy);
	}
	/*else {
		blog(LOG_INFO, "FAAACK IS IT NULL!!!");
	}*/

	UNUSED_PARAMETER(effect);
}

void RGBCBKAPI getframe(HWND hWnd, HRGB hRGB, LPBITMAPINFOHEADER bitinfo, void *pBitmapBits, ULONG_PTR userData) {
	struct rgbeasy_src *srcdata = (struct rgbeasy_src *)userData;

	if (!obs_source_showing(srcdata->src))
		return;

	//blog(LOG_INFO, "We're here");

	uint64_t start = 0, end = 0;
	uint32_t i = 0;

	float time1, time2;

	if (!capture_active || !pBitmapBits)
		return;

	start = os_gettime_ns();

	end = start - srcdata->lastframe;
	time1 = (float)end / 1000000.0f;
	time2 = 1000.0f / time1;

	srcdata->lastframe = start;	

	unsigned long width, height;

	if (pBitmapBits == NULL) return;

	if (bitinfo->biWidth == 0 || -(bitinfo->biHeight) == 0)
		return;
	//else
//		blog(LOG_INFO, "And we keep going.");

	width = bitinfo->biWidth;
	height = -(bitinfo->biHeight);
	set_frame_crap(&srcdata->video_frame, width, height);
	srcdata->cx = width;
	srcdata->cy = height;
	obs_data_set_int(srcdata->settings, "output_width", width);
	obs_data_set_int(srcdata->settings, "output_height", height);

	if (width != srcdata->cx || height != srcdata->cy)
		blog(LOG_INFO, "Allocating %lld x %lld * 4 bytes (%lld)", width, height, width * height * 4);
	unsigned int *tmpbuf = bzalloc((size_t)width * (size_t)height * 4);
	if (!tmpbuf)
		return;

	//blog(LOG_INFO, "Looping: %ld x %ld", width, height);
	for (i = 0; i + 16 <= (width * height); i += 16) {
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
	while (i < width * height) {
		memcpy(&tmpbuf[i], (uint8_t*)pBitmapBits + (i * 3), 3);
		i++;
	}


	srcdata->video_frame.data[0] = (uint8_t *)tmpbuf;
	srcdata->video_frame.timestamp = (uint64_t)start;
	obs_source_output_video2(srcdata->src, &srcdata->video_frame);

	bfree(tmpbuf);

	goto baller;

//skip_frame:;
	//blog(LOG_WARNING, "Skipping frame for some reason.");
baller:;
	//obs_leave_graphics();
}

void RGBCBKAPI nosignal(HWND hWnd, HRGB hRGB, ULONG_PTR userData) {
	// No signal stuff
	struct rgbeasy_src* srcdata = (struct rgbeasy_src*)userData;

	obs_source_output_video(srcdata->src, NULL);
}

void RGBCBKAPI valuechanged(HWND hWnd, HRGB hrgb, PRGBVALUECHANGEDINFO value_info, ULONG_PTR userData)
{
	struct rgbeasy_src* srcdata = (struct rgbeasy_src*)userData;

	long width = 0, height = 0;

	if (value_info->CaptureWidth.BChanged) {
		blog(LOG_INFO, "Capture width changed to %ld", value_info->CaptureWidth.Value);
		obs_data_set_int(srcdata->settings, "output_width", value_info->CaptureWidth.Value);
	}
	if (value_info->CaptureHeight.BChanged) {
		blog(LOG_INFO, "Capture height changed to %ld", value_info->CaptureHeight.Value);
		obs_data_set_int(srcdata->settings, "output_height", value_info->CaptureHeight.Value);
	}
	if (value_info->HorScale.BChanged) {
		blog(LOG_INFO, "HSize changed to %ld", value_info->HorScale.Value);
		obs_data_set_int(srcdata->settings, "capture_width", value_info->HorScale.Value);
	}
	if (value_info->HorPosition.BChanged) {
		blog(LOG_INFO, "HPos changed to %ld", value_info->HorPosition.Value);
		obs_data_set_int(srcdata->settings, "position_x", value_info->HorPosition.Value);
	}
	if (value_info->VerPosition.BChanged) {
		blog(LOG_INFO, "VPos changed to %ld", value_info->VerPosition.Value);
		obs_data_set_int(srcdata->settings, "position_y", value_info->VerPosition.Value);
	}
	if (value_info->Phase.BChanged) {
		blog(LOG_INFO, "Phase changed to %ld", value_info->Phase.Value);
		obs_data_set_int(srcdata->settings, "capture_phase", value_info->Phase.Value);
	}
	if (value_info->BlackLevel.BChanged) {
		blog(LOG_INFO, "Black level changed to %ld", value_info->BlackLevel.Value);
		obs_data_set_int(srcdata->settings, "black_level", value_info->BlackLevel.Value);
	}
	if (value_info->Brightness.BChanged) {
		blog(LOG_INFO, "Brightness changed to %ld", value_info->Brightness.Value);
		obs_data_set_int(srcdata->settings, "brightness", value_info->Brightness.Value);
	}
	if (value_info->Contrast.BChanged) {
		blog(LOG_INFO, "Contrast changed to %ld", value_info->Contrast.Value);
		obs_data_set_int(srcdata->settings, "contrast", value_info->Contrast.Value);
	}

	rgbeasy_update((void*)srcdata, srcdata->settings);
}