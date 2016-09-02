/*
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/delay.h>
#include <mach/gpio.h>
#include "lcdc_backlight_ic.h"

static int lcd_brightness = -1;

#if defined(CONFIG_MACH_NEVIS3G)
struct brt_value brt_table_aat[] = {
  { 255, 1 }, /* Max */
	{ 243, 3 },
	{ 231, 5 },
	{ 219, 7 },
	{ 207, 9 },
	{ 195, 11 },
	{ 183, 12 },
	{ 171, 13 },
	{ 159, 14 },
	{ 145, 15 },
	{ 130, 16 },/* default */
	{ 119, 17 },
	{ 108, 18 },
	{ 96, 21 },
	{ 85, 24 },
	{ 73, 26 },
	{ 60, 28 },
	{ 45, 29 },
	{ 30, 32 }, /* Min */
	{ 20, 32 }, /* Dimming */
	{ 0, 32 }, /* Off */
};
#else
struct brt_value brt_table_aat[] = {
		{ 255,	9  }, /* Max */
		{ 240,	10 },
		{ 230,	11 },
		{ 220,	12 },
		{ 210,	13 },
		{ 200,	14 },
		{ 190,	15 },
		{ 180,	16 },
		{ 170,	17 },
		{ 160,	18 },
		{ 150,	19 },
		{ 141,	20 }, /* default */
		{ 125,	21 },
		{ 110,	22 },
		{ 95,	23 },
		{ 80,	24 },
		{ 65,	25 },
		{ 50,	26 },
		{ 30,	27 }, /* Min */
		{ 20,	29 }, /* Dimming */
		{ 0,	30 }, /* Off */
};
#endif

#define MAX_BRT_STAGE_AAT (int)(sizeof(brt_table_aat)/sizeof(struct brt_value))

static DEFINE_SPINLOCK(bl_ctrl_lock);

#if defined(CONFIG_BACKLIGHT_AAT1401)
void aat1401_set_brightness(int level)
{
	int tune_level = 0;
	int i;

	spin_lock(&bl_ctrl_lock);
	if (level > 0) {
		if (level < MIN_BRIGHTNESS_VALUE) {
			tune_level = AAT_DIMMING_VALUE; /* DIMMING */
		} else {
			for (i = 0; i < MAX_BRT_STAGE_AAT; i++) {
				if (level <= brt_table_aat[i].level
					&& level > brt_table_aat[i+1].level) {
					tune_level = brt_table_aat[i].tune_level;
					break;
				}
			}
		}
	} /*  BACKLIGHT is KTD model */

	if (!tune_level) {
		gpio_set_value(GPIO_BL_CTRL, 0);
		mdelay(3);
	} else {
		for (; tune_level > 0; tune_level--) {
			gpio_set_value(GPIO_BL_CTRL, 0);
			udelay(3);
			gpio_set_value(GPIO_BL_CTRL, 1);
			udelay(3);
		}
	}
	mdelay(1);
	spin_unlock(&bl_ctrl_lock);
}
#endif

#if defined(CONFIG_BACKLIGHT_KTD253)
void ktd253_set_brightness(int level)
{
	int pulse;
	int tune_level = 0;
	int i;
	
	spin_lock(&bl_ctrl_lock);
	if (level > 0) {
		if (level < MIN_BRIGHTNESS_VALUE) {
			tune_level = AAT_DIMMING_VALUE; /* DIMMING */
		} else {
			for (i = 0; i < MAX_BRT_STAGE_AAT; i++) {
				if (level <= brt_table_aat[i].level
					&& level > brt_table_aat[i+1].level) {
					tune_level = brt_table_aat[i].tune_level;
					break;
				}
			}
		}
	} /*  BACKLIGHT is KTD model */
	
//// for debugging backlight temporalily. so later should be removed.
	printk("[KTD253] level : %d, tune_level : %d, lcd_brightness : %d \n",level,tune_level,lcd_brightness);	
	if(tune_level != lcd_brightness)
	{
		if (!tune_level) {
			gpio_set_value(GPIO_BL_CTRL, 0);
			mdelay(3);
			lcd_brightness = tune_level;
		} else {
			if (unlikely(lcd_brightness < 0)) {
				int val = gpio_get_value(GPIO_BL_CTRL);
				lcd_brightness = 0;
				if (val) {
					lcd_brightness = 0;
				gpio_set_value(GPIO_BL_CTRL, 0);
				mdelay(3);
					printk(KERN_INFO "LCD Baklight init in boot time on kernel\n");
				}
			}
			if (!lcd_brightness) {
				gpio_set_value(GPIO_BL_CTRL, 1);
				udelay(3);
				lcd_brightness = 1;  //make 32 levels
			}

			pulse = (tune_level - lcd_brightness + MAX_BRIGHTNESS_IN_BLU)
							% MAX_BRIGHTNESS_IN_BLU;

			for (; pulse > 0; pulse--) {
				gpio_set_value(GPIO_BL_CTRL, 0);
				udelay(3);
				gpio_set_value(GPIO_BL_CTRL, 1);
				udelay(3);
			}

			lcd_brightness = tune_level;
		}
	}
	mdelay(1);
	spin_unlock(&bl_ctrl_lock);
}
#endif

void backlight_ic_set_brightness(int level)
{
#if defined(CONFIG_BACKLIGHT_KTD253) //ARUBA, ARUBADUOS, KYLEPLUS
	ktd253_set_brightness(level);
#endif
#if defined(CONFIG_BACKLIGHT_AAT1401)
	aat1401_set_brightness(level);
#endif
}
