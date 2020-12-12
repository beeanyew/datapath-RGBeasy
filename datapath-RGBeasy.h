static const char* rgbeasy_getname(void* type_data);
static uint32_t rgbeasy_getwidth(void *data);
static uint32_t rgbeasy_getheight(void *data);

static obs_properties_t *rgbeasy_properties(void *unused);
static void rgbeasy_defaults(obs_data_t* settings);
static void rgbeasy_update(void *data, obs_data_t *settings);

static void rgbeasy_render(void *data, gs_effect_t *effect);

static void *rgbeasy_create(obs_data_t *settings, obs_source_t *source);
static void rgbeasy_destroy(void *data);

void RGBCBKAPI getframe(HWND hWnd, HRGB hRGB, LPBITMAPINFOHEADER pBitmapInfo, void *pBitmapBits, ULONG_PTR userData);
void RGBCBKAPI nosignal(HWND hWnd, HRGB hRGB, ULONG_PTR userData);
void RGBCBKAPI valuechanged(HWND hWnd, HRGB hRGB, PRGBVALUECHANGEDINFO pValueChangedInfo, ULONG_PTR userData);

void reset_capture_settings(struct rgbeasy_src *srcdata);
void set_capture_defaults(struct rgbeasy_src *srcdata);

struct rgbeasy_src {
	gs_texture_t *textures[16];

	gs_texture_t *cur_tex;

	uint32_t cx, cy;

	uint32_t *texbuf, cur_texture;
	gs_vertbuffer_t *vbuf;

	uint8_t cur_input;
	uint64_t lastframe;

	HRGB hrgb;

	bool newlycreated;
	struct obs_source_frame2 video_frame;

	gs_effect_t *draw_effect;
	obs_source_t *src;
	obs_data_t *settings;
};
