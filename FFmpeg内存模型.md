![image-20230925205400604](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230925205400604.png)

 

![image-20230925210359169](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230925210359169.png)  

![image-20230925210908753](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230925210908753.png)

​          ![image-20230925211325735](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230925211325735.png) 

![image-20230925211428140](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230925211428140.png)



![image-20230925212915153](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230925212915153.png)

**步骤4.**编解码器和上下文结构体相关联，防止多路解码冲突。

多个解码器内部由链表构成，其他音频、视频等都有自己的链表

![image-20230925214632594](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230925214632594.png)

![image-20230925215009556](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230925215009556.png)

# FFmpeg内存模型

![image-20230926164017272](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926164017272.png)

![image-20230926164508919](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926164508919.png)

 ![image-20230926164759922](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926164759922.png)

 ![image-20230926173416121](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926173416121.png)

不会冲突，因为内部由条件判断，buf会被置空

![image-20230926174318244](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926174318244.png)

init会把buf的内存地址初始化为0x00置空.接下来free会找不到之前的地址从而内存泄露

![image-20230926174649727](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926174649727.png)

move内部调用了init

--------------------------

  

![image-20230926180642242](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926180642242.png)

1024个采样点，format是2个字节16bit表示一个采样点，且是单声道，那么需要buf->size=2048个字节表示1024个采样点

buf_szie和采样点，音频格式、声道格式都有关系。

![image-20230926181235751](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926181235751.png)



