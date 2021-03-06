#ifndef __NW_CONFIG_H__
#define __NW_CONFIG_H__

#define BEACAM_INTERVAL_S	2		/**< 广播桢周期 */
#define BEACAM_TIMES			4		/**< 每次广播是发送的次数 */
#define CLIENT_MAX_NUM		100		/**< 支持的最大终端数 */

#define AP_UP_PORT			21147 /**< AP 上行端口*/
#define CLIENT_START_PORT	21149 /**< 终端接收数据的端口 = 起始端口 + 自己的ID */

#endif
