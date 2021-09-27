#include "arkanoPi.h"

int flags = 0;

TipoSistema sistema;

// Declaracion del objeto teclado
TipoTeclado teclado = {
		.columnas = {
				GPIO_KEYBOARD_COL_1,
				GPIO_KEYBOARD_COL_2,
				GPIO_KEYBOARD_COL_3,
				GPIO_KEYBOARD_COL_4,
		},
		.filas = {
				GPIO_KEYBOARD_ROW_1,
				GPIO_KEYBOARD_ROW_2,
				GPIO_KEYBOARD_ROW_3,
				GPIO_KEYBOARD_ROW_4,
		},
		.rutinas_ISR = {
				teclado_fila_1_isr,
				teclado_fila_2_isr,
				teclado_fila_3_isr,
				teclado_fila_4_isr,
		},

		// A completar por el alumno...
		// ...
};

// Declaracion del objeto display
TipoLedDisplay led_display = {
		.pines_control_columnas = {
				GPIO_LED_DISPLAY_COL_1,
				GPIO_LED_DISPLAY_COL_2,
				GPIO_LED_DISPLAY_COL_3,
		},
		.filas = {
				GPIO_LED_DISPLAY_ROW_1,
				GPIO_LED_DISPLAY_ROW_2,
				GPIO_LED_DISPLAY_ROW_3,
				GPIO_LED_DISPLAY_ROW_4,
				GPIO_LED_DISPLAY_ROW_5,
				GPIO_LED_DISPLAY_ROW_6,
				GPIO_LED_DISPLAY_ROW_7,
		},
};

//------------------------------------------------------
// FUNCIONES DE CONFIGURACION/INICIALIZACION
//------------------------------------------------------

// int ConfiguracionSistema (TipoSistema *p_sistema): procedimiento de configuracion
// e inicializacion del sistema.
// Realizará, entra otras, todas las operaciones necesarias para:
// configurar el uso de posibles librerías (e.g. Wiring Pi),
// configurar las interrupciones externas asociadas a los pines GPIO,
// configurar las interrupciones periódicas y sus correspondientes temporizadores,
// la inicializacion de los diferentes elementos de los que consta nuestro sistema,
// crear, si fuese necesario, los threads adicionales que pueda requerir el sistema
// como el thread de exploración del teclado del PC
int ConfiguraInicializaSistema (TipoSistema *p_sistema) {
	int result = 0;

	// Inicializa la librer�a de wiringPi
	if (wiringPiSetupGpio () < 0) {
		piLock (STD_IO_BUFFER_KEY);
		printf ("Unable to setup wiringPi\n");
		piUnlock (STD_IO_BUFFER_KEY);
		return -1;
	}


	// Lanzamos thread para exploracion del teclado convencional del PC
	result = piThreadCreate (thread_explora_teclado_PC);
	InicializaLedDisplay (&led_display);
	InicializaTeclado(&teclado);

	if (result != 0) {
		printf ("Thread didn't start!!!\n");
		return -1;
	}

	return result;
}

//------------------------------------------------------
// FUNCIONES LIGADAS A THREADS ADICIONALES
//------------------------------------------------------

PI_THREAD (thread_explora_teclado_PC) {
	int teclaPulsada;

	while(1) {
		delay(10); // Wiring Pi function: pauses program execution for at least 10 ms

		piLock (STD_IO_BUFFER_KEY);

		if(kbhit()) {
			teclaPulsada = kbread();

			switch(teclaPulsada) {

			case 'a':
				piLock (SYSTEM_FLAGS_KEY);
				flags |= FLAG_MOV_IZQUIERDA;
				flags |= FLAG_BOTON;
				piUnlock (SYSTEM_FLAGS_KEY);

				//printf("Tecla a pulsada!\n");
				//fflush(stdout);
				break;

			case 'd':
				piLock (SYSTEM_FLAGS_KEY);
				flags |= FLAG_MOV_DERECHA;
				flags |= FLAG_BOTON;
				piUnlock (SYSTEM_FLAGS_KEY);

				//printf("Tecla d pulsada!\n");
				//fflush(stdout);
				break;


			case 'c':
				piLock (SYSTEM_FLAGS_KEY);
				flags |= FLAG_BOTON;
				flags |= FLAG_TIMER_JUEGO;
				piUnlock (SYSTEM_FLAGS_KEY);

				//printf("Tecla c pulsada!\n");
				//fflush(stdout);
				break;

			case 'q':
				exit(0);
				break;

			default:
				printf("INVALID KEY!!!\n");
				break;
			}
		}

		piUnlock (STD_IO_BUFFER_KEY);
	}
}

// wait until next_activation (absolute time)
void delay_until (unsigned int next) {
	unsigned int now = millis();
	if (next > now) {
		delay (next - now);
	}
}



int main () {
	TipoSistema sistema;
	sistema.arkanoPi.tmr_refresco_arkano = tmr_new (tmr_actualizacion_juego_isr); // Creo el temporizador

	unsigned int next;

	// Maquina de estados: lista de transiciones
	// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
	fsm_trans_t arkanoPi[] = {
			{ WAIT_START, CompruebaBotonPulsado, WAIT_NEXT, InicializaJuego },
			{ WAIT_NEXT, CompruebaTimeoutActualizacionJuego, WAIT_NEXT, ActualizarJuego },
			{ WAIT_NEXT, CompruebaMovimientoIzquierda, WAIT_NEXT, MuevePalaIzquierda },
			{ WAIT_NEXT, CompruebaMovimientoDerecha, WAIT_NEXT, MuevePalaDerecha },
			{ WAIT_NEXT, CompruebaFinalJuego, WAIT_END, FinalJuego },
			{ WAIT_END,  CompruebaBotonPulsado, WAIT_START, ReseteaJuego },
			{-1, NULL, -1, NULL },
	};

	// Configuracion e incializacion del sistema
	ConfiguraInicializaSistema (&sistema);

	sistema.arkanoPi.p_pantalla = &(led_display.pantalla);

	fsm_t* arkanoPi_fsm = fsm_new (WAIT_START, arkanoPi, &(sistema.arkanoPi));
	fsm_t* teclado_fsm = fsm_new (TECLADO_ESPERA_COLUMNA, fsm_trans_excitacion_columnas, &teclado);
	fsm_t* tecla_fsm = fsm_new (TECLADO_ESPERA_TECLA, fsm_trans_deteccion_pulsaciones, &teclado);
	fsm_t* led_Display_fsm = fsm_new (DISPLAY_ESPERA_COLUMNA, fsm_trans_excitacion_display, &(led_display));

	ReseteaJuego(arkanoPi_fsm);


	next = millis();
	while (1) {
		fsm_fire (arkanoPi_fsm);
		fsm_fire (teclado_fsm);
		fsm_fire (tecla_fsm);
		fsm_fire (led_Display_fsm);

		next += CLK_MS;
		delay_until (next);
	}

	//Destruyo el temporizador

	fsm_destroy (arkanoPi_fsm);
	fsm_destroy (teclado_fsm);
	fsm_destroy (tecla_fsm);
	fsm_destroy (led_Display_fsm);
	tmr_destroy ((tmr_t*)(arkanoPi_fsm->user_data));

	return 0;
}

