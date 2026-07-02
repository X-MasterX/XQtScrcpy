import pytesseract; from PIL import Image; print(pytesseract.image_to_string(Image.open('/tmp/file_attachments/Screenshot_2026-07-02-15-11-36-292_com.github.android.jpg')))
