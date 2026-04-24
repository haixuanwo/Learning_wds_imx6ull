
CC=gcc
ACROSS_COMPILE=arm-buildroot-linux-gnueabihf-

all:
	$(CC) -o product_consumer product_consumer.c -lpthread
	$(ACROSS_COMPILE)$(CC) -o imx6ull_product_consumer product_consumer.c -lpthread

	$(CC) -o product_consumer1 product_consumer1.c -lpthread
	$(ACROSS_COMPILE)$(CC) -o imx6ull_product_consumer1 product_consumer1.c -lpthread

clean:
	rm -f product_consumer
	rm -f imx6ull_product_consumer
	rm -f product_consumer1
	rm -f imx6ull_product_consumer1

