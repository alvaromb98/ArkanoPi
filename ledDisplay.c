#include "ledDisplay.h"

tipo_pantalla pantalla_inicial = {
	.matriz = {
	{0,0,0,0,0,0,0,0},
	{0,1,1,0,0,1,1,0},
	{0,1,1,0,0,1,1,0},
	{0,0,0,0,0,0,0,0},
	{0,1,0,0,0,0,1,0},
	{0,0,1,1,1,1,0,0},
	{0,0,0,0,0,0,0,0},
	}
};

tipo_pantalla pantalla_final = {
	.matriz = {
	{0,0,0,0,0,0,0,0},
	{0,1,1,0,0,1,1,0},
	{0,1,1,0,0,1,1,0},
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,1,1,0,0},
	{0,1,0,0,0,0,1,0},
	{0,0,0,0,0,0,0,0},
	}
};

// Maquina de estados: lista de transiciones
// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
fsm_trans_t fsm_trans_excitacion_display[] = {
	{ DISPLAY_ESPERA_COLUMNA, CompruebaTimeoutColumnaDisplay, DISPLAY_ESPERA_COLUMNA, ActualizaExcitacionDisplay },
	{-1, NULL, -1, NULL },
};

//------------------------------------------------------
// PROCEDIMIENTOS DE INICIALIZACION DE LOS OBJETOS ESPECIFICOS
//------------------------------------------------------

void InicializaLedDisplay (TipoLedDisplay *led_display) {
	// Comenzamos excitaci�n por primera columna

	int i=0;
	int j=0;

	led_display->p_columna=-1;
	led_display->flags=0;

	for(i=0; i<NUM_FILAS_DISPLAY; i++ ){ //Recorrido de las filas de los pines de la matriz de leds
		pinMode (led_display->filas[i], OUTPUT);
		digitalWrite(led_display->filas[i], HIGH);
	}

	for(j=0; j<NUM_PINES_CONTROL_COLUMNAS_DISPLAY; j++ ){ //Recorrido de las columnas de los pines de la matriz de leds
		pinMode (led_display->pines_control_columnas[j], OUTPUT);
		digitalWrite(led_display->pines_control_columnas[j], LOW);
	}

	led_display->tmr_refresco_display=tmr_new(timer_refresco_display_isr);
	tmr_startms((led_display -> tmr_refresco_display), TIMEOUT_COLUMNA_DISPLAY);
}

//------------------------------------------------------
// OTROS PROCEDIMIENTOS PROPIOS DE LA LIBRERIA
//------------------------------------------------------

void ApagaFilas (TipoLedDisplay *led_display){

	digitalWrite(GPIO_LED_DISPLAY_ROW_1, HIGH);
	digitalWrite(GPIO_LED_DISPLAY_ROW_2, HIGH);
	digitalWrite(GPIO_LED_DISPLAY_ROW_3, HIGH);
	digitalWrite(GPIO_LED_DISPLAY_ROW_4, HIGH);
	digitalWrite(GPIO_LED_DISPLAY_ROW_5, HIGH);
	digitalWrite(GPIO_LED_DISPLAY_ROW_6, HIGH);
	digitalWrite(GPIO_LED_DISPLAY_ROW_7, HIGH);
}

void ExcitaColumnas(int columna) {

	//led_display.p_columna = columna;

	switch (columna) {

	case 0:

		digitalWrite (GPIO_LED_DISPLAY_COL_1, LOW);
		digitalWrite (GPIO_LED_DISPLAY_COL_2, LOW);
		digitalWrite (GPIO_LED_DISPLAY_COL_3, LOW);

		break;

	case 1:

		digitalWrite (GPIO_LED_DISPLAY_COL_1, HIGH);
		digitalWrite (GPIO_LED_DISPLAY_COL_2, LOW);
		digitalWrite (GPIO_LED_DISPLAY_COL_3, LOW);

		break;

	case 2:

		digitalWrite (GPIO_LED_DISPLAY_COL_1, LOW);
		digitalWrite (GPIO_LED_DISPLAY_COL_2, HIGH);
		digitalWrite (GPIO_LED_DISPLAY_COL_3, LOW);

		break;

	case 3:

		digitalWrite (GPIO_LED_DISPLAY_COL_1, HIGH);
		digitalWrite (GPIO_LED_DISPLAY_COL_2, HIGH);
		digitalWrite (GPIO_LED_DISPLAY_COL_3, LOW);

		break;

	case 4:

		digitalWrite (GPIO_LED_DISPLAY_COL_1, LOW);
		digitalWrite (GPIO_LED_DISPLAY_COL_2, LOW);
		digitalWrite (GPIO_LED_DISPLAY_COL_3, HIGH);

		break;

	case 5:

		digitalWrite (GPIO_LED_DISPLAY_COL_1, HIGH);
		digitalWrite (GPIO_LED_DISPLAY_COL_2, LOW);
		digitalWrite (GPIO_LED_DISPLAY_COL_3, HIGH);

		break;

	case 6:

		digitalWrite (GPIO_LED_DISPLAY_COL_1, LOW);
		digitalWrite (GPIO_LED_DISPLAY_COL_2, HIGH);
		digitalWrite (GPIO_LED_DISPLAY_COL_3, HIGH);

		break;

	case 7:

		digitalWrite (GPIO_LED_DISPLAY_COL_1, HIGH);
		digitalWrite (GPIO_LED_DISPLAY_COL_2, HIGH);
		digitalWrite (GPIO_LED_DISPLAY_COL_3, HIGH);

		break;
	}
}

void ActualizaLedDisplay (TipoLedDisplay *led_display) {

	//led_display->p_columna+=1;
	if(led_display->p_columna > NUM_COLUMNAS_DISPLAY){
	led_display->p_columna = 0;
	}
	ApagaFilas(led_display);
	ExcitaColumnas(led_display->p_columna);
    for(int i=0; i<NUM_FILAS_DISPLAY; i++) {
        if(led_display->pantalla.matriz[i][led_display->p_columna]){
            digitalWrite(led_display->filas[i], LOW);
		} else {
			digitalWrite(led_display->filas[i], HIGH);
		}
	}
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

int CompruebaTimeoutColumnaDisplay (fsm_t* this) {
	int result = 0;
	TipoLedDisplay *p_ledDisplay;
	p_ledDisplay = (TipoLedDisplay*)(this->user_data);

	piLock(MATRIX_KEY);
	result = (p_ledDisplay-> flags & FLAG_TIMEOUT_COLUMNA_DISPLAY);
	piUnlock(MATRIX_KEY);

	return result;
}

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

void ActualizaExcitacionDisplay (fsm_t* this) {
	TipoLedDisplay *p_ledDisplay;
	p_ledDisplay = (TipoLedDisplay*)(this->user_data);

    piLock(MATRIX_KEY);
    p_ledDisplay-> flags &= (~FLAG_TIMEOUT_COLUMNA_DISPLAY);
	piUnlock(MATRIX_KEY);

	//ApagaFilas(p_ledDisplay);
	ExcitaColumnas(p_ledDisplay->p_columna);
	ActualizaLedDisplay(p_ledDisplay); // Enciendo los leds correspondientes
	p_ledDisplay->p_columna++; // Paso a la siguiente columna

	if (p_ledDisplay->p_columna == NUM_COLUMNAS_DISPLAY) { // Si llego a las 8 columnas
		p_ledDisplay->p_columna = 0; // Vuelvo a empezar
	}
}

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------

void timer_refresco_display_isr (union sigval value) {

	piLock(MATRIX_KEY);
	led_display.flags |= FLAG_TIMEOUT_COLUMNA_DISPLAY;
	piUnlock(MATRIX_KEY);

	tmr_startms(led_display.tmr_refresco_display, TIMEOUT_COLUMNA_DISPLAY);
}
