/*****************************************************************************
* @FileName:Demo.cpp
* @Descriptions: 简单的应用程序
* @Version: ver 1.0

*****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <pthread.h> 
#include <iostream>
#include "DVPCamera.h"		/* DVP API 依赖 */

using namespace std;
#define TEST_TRIG			/* 触发开�?*/
#define SOFT_TRIG			/* 软触�?*/
#define GRABCOUNT 20		/* 抓帧次数 */

void* test(void *p)
{
	dvpStatus status;
	dvpHandle h;
	bool trigMode = false;
	char *name = (char*)p;

	printf("test start,camera is %s\r\n", name);
	do 
	{
		/* 打开设备 */
		status = dvpOpenByName(name, OPEN_NORMAL, &h);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpOpenByName failed with err:%d\r\n", status);
			break;
		}

		dvpRegion region;
		double exp;
		float gain;

#ifdef TEST_TRIG
		/* 设置触发模式*/
		status = dvpSetTriggerState(h, true);
		if (status == DVP_STATUS_OK)
		{
			dvpSetTriggerSource(h, TRIGGER_SOURCE_SOFTWARE);
			dvpSetTriggerInputType(h, TRIGGER_POS_EDGE);
			dvpSetInputIoFunction(h, INPUT_IO_1, INPUT_FUNCTION_TRIGGER);
			trigMode = true;
		}
		else
		{
			printf("dvpSetTriggerState failed with err:%d\r\n", status);
			break;
		}
#endif
		/* 打印ROI信息 */
		status = dvpGetRoi(h, &region);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpGetRoi failed with err:%d\r\n", status);
			break;
		}
		printf("%s, region: x:%d, y:%d, w:%d, h:%d\r\n", name, region.X, region.Y, region.W, region.H);

		/* 打印曝光增益信息 */
		status = dvpGetExposure(h, &exp);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpGetExposure failed with err:%d\r\n", status);
			break;
		}

		status = dvpGetAnalogGain(h, &gain);
		if (status != DVP_STATUS_OK)
		{
			printf("dvpGetAnalogGain failed with err:%d\r\n", status);
			break;
		}

		printf("%s, exposure: %lf, gain: %f\r\n", name, exp, gain);


		uint32_t v;
		/* 帧信�?*/
		dvpFrame frame;
		/* 帧数据首地址，用户不需要申请释放内�?*/
		void *p;

		/* 开始视频流 */
		status = dvpStart(h);
		if (status != DVP_STATUS_OK)
		{
			break;
		}

		/* 抓帧 */
		for (int j = 0; j < GRABCOUNT; j++)
		{
#ifdef SOFT_TRIG
			if (trigMode)
			{
				// 触发一�?
				status = dvpTriggerFire(h);
				if (status != DVP_STATUS_OK)
				{
					printf("Fail to trig a frame\r\n");           
				}
			}
#endif
			/* 当前案例没有设置相机的曝光增益等参数，只展示在默认的ROI区域显示帧信�?*/
			status = dvpGetFrame(h, &frame, &p, 3000);
			if (status != DVP_STATUS_OK)
			{
				if (trigMode)
					continue;
				else
					break;
			}
			
			/* 显示帧数和帧�?*/
			dvpFrameCount framecount;
			status = dvpGetFrameCount(h, &framecount);
			if(status != DVP_STATUS_OK)
			{
				printf("get framecount failed\n");
			}
			printf("framecount: %d, framerate: %f\n", framecount.uFrameCount, framecount.fFrameRate);

			/* 显示帧信�?*/
			printf("%s, frame:%lu, timestamp:%lu, %d*%d, %dbytes, format:%d\r\n",
				name,
				frame.uFrameID,
				frame.uTimestamp,
				frame.iWidth,
				frame.iHeight,
				frame.uBytes,
				frame.format);

			/* 需要创建pic目录保存图片 */
			// char PicName[64];
			// 
			// 				sprintf(PicName, "%s_pic_%d.jpg",name, k);
			// 				status = dvpSavePicture(&frame, p, PicName, 90);
			// 				if (status == DVP_STATUS_OK)
			// 				{
			// 					printf("Save to %s OK\r\n", PicName);
			// 				}
		}        

		/* 停止视频�?*/
		status = dvpStop(h);
		if (status != DVP_STATUS_OK)
		{
			break;
		}
	}while(0);

	dvpClose(h);

	printf("test quit, %s, status:%d\r\n", name, status);
}


int main(int argc, char *argv[])
{
	printf("start...\r\n");

	dvpUint32 count = 0;
  dvpInt32 num = -1;
	dvpCameraInfo info[8];   
	dvpStatus status;
  std::string strInput;

	/* 枚举设备 */
	dvpRefresh(&count);
	if (count > 8)
		count = 8;

	for (int i = 0; i < count; i++)
	{    
		if(dvpEnum(i, &info[i]) == DVP_STATUS_OK)    
		{        
			printf("[%d]-Camera FriendlyName : %s\r\n", i, info[i].FriendlyName);
		}
	}

	/* 没发现设�?*/
	if (count == 0)
		return 0;

	printf("Please enter a valid number of the camera you want to open:\r\n");
  cin.clear();
  cin >> strInput;
    
  try
  {
    num = std::stoi(strInput);
    printf("num:[%d]\r\n", num);
  }
  catch(...)
  {
    printf("the number[%s] of the camera is invalid\r\n", strInput.c_str());
  }    
   
  if(num < 0 || num >= count)
  {
    printf("the number of the camera is invalid num=%d \r\n", num);
    return 0;
	} 


	pthread_t tidp;
	int r = pthread_create(&tidp, NULL, test, (void*)info[num].FriendlyName); 
	pthread_join(tidp, NULL);

	return 0;
}

