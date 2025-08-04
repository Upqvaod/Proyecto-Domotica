import cv2
import torch
from ultralytics import YOLO
import numpy as np
import serial
import time
import threading

arduino = serial.Serial('COM10', 9600)
time.sleep(2)  # Esperar que la conexión se establezca

modelo = YOLO("yolov8n.pt")  # Puedes usar 'yolov8s.pt' para más precisión

# URL de la cámara IP del celular (ajusta según la tuya)
#url = 'http://192.168.0.12:8080/shot.jpg'
url = 0

# Variable para evitar envíos repetitivos
persona_detectada = False

def procesar_video():
    global persona_detectada

    cap = cv2.VideoCapture(url)

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error al capturar la imagen.")
            continue

        # Reducir tamaño de la imagen para mejorar velocidad
        frame = cv2.resize(frame, (480, 360))

        # Realizar detección de personas con YOLOv8
        resultados = modelo(frame)

        persona_presente = False  # Variable para verificar detección en este frame

        for r in resultados:
            for box in r.boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0])  # Coordenadas del ROI
                class_id = int(box.cls[0])  # ID de la clase detectada
                confianza = float(box.conf[0])  # Confianza del modelo

                # Filtrar solo personas (clase 0 en COCO)
                if class_id == 0 and confianza > 0.5:
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                    cv2.putText(frame, f"Persona {confianza:.2f}", (x1, y1 - 10),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
                    persona_presente = True

        # Enviar comando al Arduino solo si cambia el estado
        if persona_presente and not persona_detectada:
            arduino.write(b'M')  # Enviar comando 'M' (Encender LED)
            print("Persona detectada - LED ENCENDIDO")
            persona_detectada = True

        elif not persona_presente and persona_detectada:
            arduino.write(b'S')  # Enviar comando 'S' (Apagar LED)
            print("No hay persona - LED APAGADO")
            persona_detectada = False

        # Mostrar la imagen procesada en OpenCV
        cv2.imshow('Detección de Personas - YOLOv8', frame)

        # Presionar 'Esc' para salir
        if cv2.waitKey(1) & 0xFF == 27:
            break

    cap.release()
    cv2.destroyAllWindows()
    arduino.close()

# Ejecutar en un hilo separado para evitar bloqueos
thread = threading.Thread(target=procesar_video)
thread.start()