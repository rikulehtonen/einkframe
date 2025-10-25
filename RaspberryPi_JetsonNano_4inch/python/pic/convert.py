from PIL import Image

image = Image.open('test1.png')

pal_image = Image.new("P", (1,1))
pal_image.putpalette( (0,0,0,  255,255,255,  255,255,0,  255,0,0,  0,0,0,  0,0,255,  0,255,0) + (0,0,0)*249)


image_6color = image.convert("RGB").quantize(palette=pal_image)
#buf_6color = bytearray(image_6color.tobytes('raw'))

image_6color.save("test1.bmp")
