#include <stdbool.h>
#include <stdint.h>

// Shared notification result values for modal overlay prompts.
typedef enum NotificationResult {
	NOTIFICATION_RESULT_NONE = 0,
	NOTIFICATION_RESULT_LEFT = 1,
	NOTIFICATION_RESULT_RIGHT = 2,
} NotificationResult;

extern float maxdepthinmm;
extern float speedlimit;
extern bool dark_mode;

#ifdef __cplusplus
extern "C" {
#endif

void xtoysMenuButtonRToggle(void);

#ifdef __cplusplus
}

int showNotification(const char *title,
                     const char *text,
                     uint32_t duration,
                     bool showLeftButton = false,
                     const char *leftButtonText = nullptr,
                     bool showRightButton = false,
                     const char *rightButtonText = nullptr);
#endif



// UI Colors

