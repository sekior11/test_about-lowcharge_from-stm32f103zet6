from pdf2docx import Converter

JOBS = [
    (r"C:\Users\22743\Desktop\stm32project\sea_sensor\IWT603产品规格书.pdf",
     r"C:\Users\22743\Desktop\stm32project\sea_sensor\IWT603产品规格书.docx"),
    (r"C:\Users\22743\Desktop\stm32project\sea_sensor\通讯协议.pdf",
     r"C:\Users\22743\Desktop\stm32project\sea_sensor\通讯协议.docx"),
]

for pdf_path, docx_path in JOBS:
    print(f"[start] {pdf_path}")
    cv = Converter(pdf_path)
    cv.convert(docx_path)
    cv.close()
    print(f"[done]  -> {docx_path}")
