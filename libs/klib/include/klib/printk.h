#ifndef _INCLUDE_LINUX_PRINTK_H_
#define _INCLUDE_LINUX_PRINTK_H_


#define KERN_EMERG	"<0>"	/* system is unusable			*/
#define KERN_ALERT	"<1>"	/* action must be taken immediately	*/
#define KERN_CRIT	"<2>"	/* critical conditions			*/
#define KERN_ERR	"<3>"	/* error conditions			*/
#define KERN_WARNING	"<4>"	/* warning conditions			*/
#define KERN_NOTICE	"<5>"	/* normal but significant condition	*/
#define KERN_INFO	"<6>"	/* informational			*/
#define KERN_DEBUG	"<7>"	/* debug-level messages			*/
#define KERN_CNT	"<c>"	/* continue the last printed message	*/
#define KERN_STATUS 	"<8>" 	/* Update status line. Do not use this in threaded context */

int printk(const char *format, ...);

void printk_set_log_level(int log_level);

#endif /* _INCLUDE_LINUX_PRINTK_H_ */
