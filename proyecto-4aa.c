
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_NODOS 12
#define GLADE_FILE "proyecto-4aa.glade"

typedef enum {
    NO_DIRIGIDO,
    DIRIGIDO
} TipoGrafo;

typedef struct {
    int x;
    int y;
} Coordenada;

typedef struct {
    int K;
    TipoGrafo tipo;
    int matriz_adyacencia[MAX_NODOS][MAX_NODOS];
    Coordenada posiciones[MAX_NODOS];
} Grafo;

static Grafo grafo_actual;
static GtkBuilder *builder;
static GtkWidget *window_main;
static GtkWidget *grid_matriz;
static GtkWidget *grid_posiciones;
static GtkWidget *spin_num_nodes;
static GtkWidget *radio_no_dirigido;
static GtkWidget *radio_dirigido;
static GtkWidget *scroll_matriz;
static GtkWidget *notebook_main;
static GtkEntry *matriz_entries[MAX_NODOS][MAX_NODOS];
static GtkSpinButton *pos_x_spins[MAX_NODOS];
static GtkSpinButton *pos_y_spins[MAX_NODOS];
static int num_nodos_actual = 0;

void limpiar_matriz();
void limpiar_posiciones();
void crear_matriz_ui(int K);
void crear_posiciones_ui(int K);
void actualizar_matriz_simetrica(int fila, int col);
bool validar_posiciones();
bool tiene_ciclo_hamiltoniano();
bool tiene_ruta_hamiltoniana();
bool encontrar_ciclo_hamiltoniano(int *secuencia, int *longitud);
bool encontrar_ruta_hamiltoniana(int *secuencia, int *longitud);
bool es_euleriano();
bool es_semieuleriano();
int encontrar_ciclo_euleriano_hierholzer(int *secuencia);
int encontrar_ciclo_euleriano_fleury(int *secuencia);
int encontrar_ruta_euleriana_fleury(int *secuencia);
void generar_latex(const char *filename);
void compilar_y_mostrar_pdf(const char *texfile);
void guardar_grafo_archivo();
void cargar_grafo_archivo();
void on_save_button_clicked(GtkButton *button, gpointer user_data);
void on_load_button_clicked(GtkButton *button, gpointer user_data);

bool validar_numero_nodos(int k) {
    return k >= 1 && k <= MAX_NODOS;
}

bool posicion_duplicada(const Coordenada *posiciones, int K, int x, int y, int excluir_idx) {
    for (int i = 0; i < K; i++) {
        if (i != excluir_idx && posiciones[i].x == x && posiciones[i].y == y) {
            return true;
        }
    }
    return false;
}

void on_num_nodes_changed(GtkSpinButton *spinbutton, gpointer user_data) {
    (void)spinbutton;
    (void)user_data;
}

void on_apply_nodes_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    int K = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_num_nodes));
    
    if (!validar_numero_nodos(K)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window_main),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "El número de nodos debe estar entre 1 y %d", MAX_NODOS);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    grafo_actual.K = K;
    num_nodos_actual = K;
    
    // Inicializar la matriz a 0 cuando se cambia el número de nodos
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < K; j++) {
            grafo_actual.matriz_adyacencia[i][j] = 0;
        }
    }
    
    // Inicializar las posiciones a 0
    for (int i = 0; i < K; i++) {
        grafo_actual.posiciones[i].x = 0;
        grafo_actual.posiciones[i].y = 0;
    }
    
    limpiar_matriz();
    limpiar_posiciones();
    crear_matriz_ui(K);
    crear_posiciones_ui(K);
}

void on_tipo_grafo_changed(GtkToggleButton *togglebutton, gpointer user_data) {
    (void)togglebutton;
    (void)user_data;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_no_dirigido))) {
        grafo_actual.tipo = NO_DIRIGIDO;
    } else {
        grafo_actual.tipo = DIRIGIDO;
    }
    
    if (num_nodos_actual > 0 && grafo_actual.tipo == NO_DIRIGIDO) {
        for (int i = 0; i < num_nodos_actual; i++) {
            for (int j = 0; j < num_nodos_actual; j++) {
                if (i != j) {
                    int valor = grafo_actual.matriz_adyacencia[i][j];
                    grafo_actual.matriz_adyacencia[j][i] = valor;
                    if (matriz_entries[j][i]) {
                        char str[2];
                        snprintf(str, 2, "%d", valor);
                        gtk_entry_set_text(matriz_entries[j][i], str);
                    }
                }
            }
        }
    }
}

void on_matriz_changed(GtkEditable *editable, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(editable);
    int *coords = (int *)user_data;
    int fila = coords[0];
    int col = coords[1];
    
    const char *text = gtk_entry_get_text(entry);
    int valor = atoi(text);
    
    if (valor != 0 && valor != 1) {
        valor = 0;
        gtk_entry_set_text(entry, "0");
    }
    
    if (fila == col) {
        valor = 0;
        gtk_entry_set_text(entry, "0");
    }
    
    grafo_actual.matriz_adyacencia[fila][col] = valor;
    
    if (grafo_actual.tipo == NO_DIRIGIDO && fila != col) {
        actualizar_matriz_simetrica(fila, col);
    }
}

void actualizar_matriz_simetrica(int fila, int col) {
    int valor = grafo_actual.matriz_adyacencia[fila][col];
    grafo_actual.matriz_adyacencia[col][fila] = valor;
    
    if (matriz_entries[col][fila]) {
        char str[16];
        snprintf(str, sizeof(str), "%d", valor);
        gtk_entry_set_text(matriz_entries[col][fila], str);
    }
}

void guardar_grafo_archivo() {
    if (num_nodos_actual == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window_main),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Primero debe configurar el número de nodos");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Actualizar las posiciones desde los spin buttons antes de guardar
    for (int i = 0; i < num_nodos_actual; i++) {
        if (pos_x_spins[i]) {
            grafo_actual.posiciones[i].x = gtk_spin_button_get_value_as_int(pos_x_spins[i]);
        }
        if (pos_y_spins[i]) {
            grafo_actual.posiciones[i].y = gtk_spin_button_get_value_as_int(pos_y_spins[i]);
        }
    }
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Guardar Grafo",
        GTK_WINDOW(window_main), GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancelar", GTK_RESPONSE_CANCEL,
        "_Guardar", GTK_RESPONSE_ACCEPT, NULL);
    
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        FILE *f = fopen(filename, "w");
        
        if (f) {
            fprintf(f, "%d\n", grafo_actual.K);
            fprintf(f, "%d\n", grafo_actual.tipo);
            
            for (int i = 0; i < grafo_actual.K; i++) {
                for (int j = 0; j < grafo_actual.K; j++) {
                    fprintf(f, "%d ", grafo_actual.matriz_adyacencia[i][j]);
                }
                fprintf(f, "\n");
            }
            
            for (int i = 0; i < grafo_actual.K; i++) {
                fprintf(f, "%d %d\n", grafo_actual.posiciones[i].x, grafo_actual.posiciones[i].y);
            }
            
            fclose(f);
            
            GtkWidget *dialog_success = gtk_message_dialog_new(GTK_WINDOW(window_main),
                GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                "Grafo guardado exitosamente en: %s", filename);
            gtk_dialog_run(GTK_DIALOG(dialog_success));
            gtk_widget_destroy(dialog_success);
        } else {
            GtkWidget *dialog_error = gtk_message_dialog_new(GTK_WINDOW(window_main),
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Error al guardar el archivo: %s", filename);
            gtk_dialog_run(GTK_DIALOG(dialog_error));
            gtk_widget_destroy(dialog_error);
        }
        
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

void on_save_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    (void)user_data;
    guardar_grafo_archivo();
}

void on_save_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    guardar_grafo_archivo();
}

void cargar_grafo_archivo() {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Cargar Grafo",
        GTK_WINDOW(window_main), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancelar", GTK_RESPONSE_CANCEL,
        "_Abrir", GTK_RESPONSE_ACCEPT, NULL);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        FILE *f = fopen(filename, "r");
        
        if (f) {
            int K, tipo;
            if (fscanf(f, "%d", &K) != 1 || fscanf(f, "%d", &tipo) != 1) {
                GtkWidget *dialog_error = gtk_message_dialog_new(GTK_WINDOW(window_main),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "Error: Formato de archivo inválido");
                gtk_dialog_run(GTK_DIALOG(dialog_error));
                gtk_widget_destroy(dialog_error);
                fclose(f);
                g_free(filename);
                gtk_widget_destroy(dialog);
                return;
            }
            
            if (validar_numero_nodos(K)) {
                grafo_actual.K = K;
                grafo_actual.tipo = (TipoGrafo)tipo;
                num_nodos_actual = K;
                
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_num_nodes), K);
                if (tipo == NO_DIRIGIDO) {
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_no_dirigido), TRUE);
                } else {
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_dirigido), TRUE);
                }
                
                for (int i = 0; i < K; i++) {
                    for (int j = 0; j < K; j++) {
                        if (fscanf(f, "%d", &grafo_actual.matriz_adyacencia[i][j]) != 1) {
                            GtkWidget *dialog_error = gtk_message_dialog_new(GTK_WINDOW(window_main),
                                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                "Error: Formato de archivo inválido en la matriz");
                            gtk_dialog_run(GTK_DIALOG(dialog_error));
                            gtk_widget_destroy(dialog_error);
                            fclose(f);
                            g_free(filename);
                            gtk_widget_destroy(dialog);
                            return;
                        }
                    }
                }
                
                for (int i = 0; i < K; i++) {
                    if (fscanf(f, "%d %d", &grafo_actual.posiciones[i].x, &grafo_actual.posiciones[i].y) != 2) {
                        GtkWidget *dialog_error = gtk_message_dialog_new(GTK_WINDOW(window_main),
                            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                            "Error: Formato de archivo inválido en las posiciones");
                        gtk_dialog_run(GTK_DIALOG(dialog_error));
                        gtk_widget_destroy(dialog_error);
                        fclose(f);
                        g_free(filename);
                        gtk_widget_destroy(dialog);
                        return;
                    }
                }
                
                limpiar_matriz();
                limpiar_posiciones();
                crear_matriz_ui(K);
                crear_posiciones_ui(K);
                
                // Bloquear señales temporalmente para evitar que los callbacks interfieran
                for (int i = 0; i < K; i++) {
                    for (int j = 0; j < K; j++) {
                        if (matriz_entries[i][j]) {
                            g_signal_handlers_block_by_func(matriz_entries[i][j], 
                                (gpointer)on_matriz_changed, NULL);
                        }
                    }
                }
                
                for (int i = 0; i < K; i++) {
                    for (int j = 0; j < K; j++) {
                        if (matriz_entries[i][j]) {
                            char str[16];
                            snprintf(str, sizeof(str), "%d", grafo_actual.matriz_adyacencia[i][j]);
                            gtk_entry_set_text(matriz_entries[i][j], str);
                        }
                    }
                }
                
                // Desbloquear señales después de actualizar
                for (int i = 0; i < K; i++) {
                    for (int j = 0; j < K; j++) {
                        if (matriz_entries[i][j]) {
                            g_signal_handlers_unblock_by_func(matriz_entries[i][j], 
                                (gpointer)on_matriz_changed, NULL);
                        }
                    }
                }
                
                for (int i = 0; i < K; i++) {
                    if (pos_x_spins[i]) {
                        gtk_spin_button_set_value(pos_x_spins[i], grafo_actual.posiciones[i].x);
                    }
                    if (pos_y_spins[i]) {
                        gtk_spin_button_set_value(pos_y_spins[i], grafo_actual.posiciones[i].y);
                    }
                }
                
                GtkWidget *dialog_success = gtk_message_dialog_new(GTK_WINDOW(window_main),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                    "Grafo cargado exitosamente desde: %s", filename);
                gtk_dialog_run(GTK_DIALOG(dialog_success));
                gtk_widget_destroy(dialog_success);
            } else {
                GtkWidget *dialog_error = gtk_message_dialog_new(GTK_WINDOW(window_main),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "Error: Número de nodos inválido en el archivo (debe estar entre 1 y %d)", MAX_NODOS);
                gtk_dialog_run(GTK_DIALOG(dialog_error));
                gtk_widget_destroy(dialog_error);
            }
            
            fclose(f);
        } else {
            GtkWidget *dialog_error = gtk_message_dialog_new(GTK_WINDOW(window_main),
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Error al abrir el archivo: %s", filename);
            gtk_dialog_run(GTK_DIALOG(dialog_error));
            gtk_widget_destroy(dialog_error);
        }
        
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

void on_load_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    (void)user_data;
    cargar_grafo_archivo();
}

void on_load_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    cargar_grafo_archivo();
}

void on_quit_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    (void)user_data;
    gtk_main_quit();
}

void on_clear_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    for (int i = 0; i < num_nodos_actual; i++) {
        for (int j = 0; j < num_nodos_actual; j++) {
            grafo_actual.matriz_adyacencia[i][j] = 0;
            if (matriz_entries[i][j]) {
                gtk_entry_set_text(matriz_entries[i][j], "0");
            }
        }
    }
    
    for (int i = 0; i < num_nodos_actual; i++) {
        grafo_actual.posiciones[i].x = 0;
        grafo_actual.posiciones[i].y = 0;
        if (pos_x_spins[i]) {
            gtk_spin_button_set_value(pos_x_spins[i], 0);
        }
        if (pos_y_spins[i]) {
            gtk_spin_button_set_value(pos_y_spins[i], 0);
        }
    }
}

void on_generate_latex_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    if (num_nodos_actual == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window_main),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Primero debe configurar el número de nodos");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    for (int i = 0; i < num_nodos_actual; i++) {
        if (pos_x_spins[i]) {
            grafo_actual.posiciones[i].x = gtk_spin_button_get_value_as_int(pos_x_spins[i]);
        }
        if (pos_y_spins[i]) {
            grafo_actual.posiciones[i].y = gtk_spin_button_get_value_as_int(pos_y_spins[i]);
        }
    }
    
    if (!validar_posiciones()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window_main),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "No puede haber dos nodos en la misma posición");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    generar_latex("proyecto-4aa.tex");
    compilar_y_mostrar_pdf("proyecto-4aa.tex");
}

void limpiar_matriz() {
    if (grid_matriz) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(grid_matriz));
        GList *iter;
        
        for (iter = children; iter != NULL; iter = iter->next) {
            GtkWidget *child = GTK_WIDGET(iter->data);
            gpointer user_data = g_object_get_data(G_OBJECT(child), "coords");
            if (user_data) {
                free(user_data);
            }
            gtk_widget_destroy(child);
        }
        
        g_list_free(children);
        
        for (int i = 0; i < MAX_NODOS; i++) {
            for (int j = 0; j < MAX_NODOS; j++) {
                matriz_entries[i][j] = NULL;
            }
        }
    }
}

void limpiar_posiciones() {
    if (grid_posiciones) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(grid_posiciones));
        GList *iter;
        
        for (iter = children; iter != NULL; iter = iter->next) {
            GtkWidget *child = GTK_WIDGET(iter->data);
            gtk_widget_destroy(child);
        }
        
        g_list_free(children);
        
        for (int i = 0; i < MAX_NODOS; i++) {
            pos_x_spins[i] = NULL;
            pos_y_spins[i] = NULL;
        }
    }
}

void crear_matriz_ui(int K) {
    for (int i = 0; i < K; i++) {
        char label[16];
        snprintf(label, sizeof(label), "%d", i);
        GtkWidget *lbl = gtk_label_new(label);
        gtk_grid_attach(GTK_GRID(grid_matriz), lbl, 0, i + 1, 1, 1);
    }
    
    for (int j = 0; j < K; j++) {
        char label[16];
        snprintf(label, sizeof(label), "%d", j);
        GtkWidget *lbl = gtk_label_new(label);
        gtk_grid_attach(GTK_GRID(grid_matriz), lbl, j + 1, 0, 1, 1);
    }
    
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < K; j++) {
            GtkWidget *entry = gtk_entry_new();
            gtk_entry_set_max_length(GTK_ENTRY(entry), 1);
            gtk_entry_set_width_chars(GTK_ENTRY(entry), 2);
            
            int *coords = malloc(2 * sizeof(int));
            coords[0] = i;
            coords[1] = j;
            
            g_object_set_data(G_OBJECT(entry), "coords", coords);
            
            // Bloquear señal temporalmente para evitar callback durante inicialización
            g_signal_connect(entry, "changed", G_CALLBACK(on_matriz_changed), coords);
            g_signal_handlers_block_by_func(entry, (gpointer)on_matriz_changed, NULL);
            
            if (i == j) {
                gtk_widget_set_sensitive(entry, FALSE);
                gtk_entry_set_text(GTK_ENTRY(entry), "0");
                grafo_actual.matriz_adyacencia[i][j] = 0;
            } else {
                // Usar el valor existente de la matriz si está disponible, sino inicializar a 0
                char str[16];
                snprintf(str, sizeof(str), "%d", grafo_actual.matriz_adyacencia[i][j]);
                gtk_entry_set_text(GTK_ENTRY(entry), str);
            }
            
            gtk_grid_attach(GTK_GRID(grid_matriz), entry, j + 1, i + 1, 1, 1);
            matriz_entries[i][j] = GTK_ENTRY(entry);
            
            // Desbloquear señal después de inicializar
            g_signal_handlers_unblock_by_func(entry, (gpointer)on_matriz_changed, NULL);
        }
    }
    
    gtk_widget_show_all(grid_matriz);
    
    if (scroll_matriz) {
        gtk_widget_queue_resize(scroll_matriz);
        gtk_widget_show_all(scroll_matriz);
    }
}

void crear_posiciones_ui(int K) {
    GtkWidget *lbl_nodo = gtk_label_new("Nodo");
    GtkWidget *lbl_x = gtk_label_new("X");
    GtkWidget *lbl_y = gtk_label_new("Y");
    
    gtk_grid_attach(GTK_GRID(grid_posiciones), lbl_nodo, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_posiciones), lbl_x, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_posiciones), lbl_y, 2, 0, 1, 1);
    
    for (int i = 0; i < K; i++) {
        char label[16];
        snprintf(label, sizeof(label), "%d", i);
        GtkWidget *lbl = gtk_label_new(label);
        gtk_grid_attach(GTK_GRID(grid_posiciones), lbl, 0, i + 1, 1, 1);
        
        // Usar el valor existente de la posición si está disponible, sino inicializar a 0
        double valor_x = grafo_actual.posiciones[i].x;
        double valor_y = grafo_actual.posiciones[i].y;
        
        GtkAdjustment *adj_x = gtk_adjustment_new(valor_x, 0, 1000, 1, 10, 0);
        GtkWidget *spin_x = gtk_spin_button_new(adj_x, 1, 0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_x), valor_x);
        gtk_grid_attach(GTK_GRID(grid_posiciones), spin_x, 1, i + 1, 1, 1);
        pos_x_spins[i] = GTK_SPIN_BUTTON(spin_x);
        
        GtkAdjustment *adj_y = gtk_adjustment_new(valor_y, 0, 1000, 1, 10, 0);
        GtkWidget *spin_y = gtk_spin_button_new(adj_y, 1, 0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_y), valor_y);
        gtk_grid_attach(GTK_GRID(grid_posiciones), spin_y, 2, i + 1, 1, 1);
        pos_y_spins[i] = GTK_SPIN_BUTTON(spin_y);
        
        // No inicializar a 0 aquí, mantener los valores existentes si ya están cargados
    }
    
    gtk_widget_show_all(grid_posiciones);
    
    GtkWidget *scroll_posiciones = gtk_widget_get_parent(grid_posiciones);
    if (scroll_posiciones) {
        gtk_widget_queue_resize(scroll_posiciones);
        gtk_widget_show_all(scroll_posiciones);
    }
}

bool validar_posiciones() {
    for (int i = 0; i < num_nodos_actual; i++) {
        if (posicion_duplicada(grafo_actual.posiciones, num_nodos_actual,
            grafo_actual.posiciones[i].x, grafo_actual.posiciones[i].y, i)) {
            return false;
        }
    }
    return true;
}

bool tiene_ciclo_hamiltoniano() {
    int K = grafo_actual.K;
    if (K < 3) return false;
    
    int camino[K];
    bool visitado[K];
    
    for (int i = 0; i < K; i++) {
        visitado[i] = false;
    }
    
    bool backtrack(int pos) {
        if (pos == K) {
            return grafo_actual.matriz_adyacencia[camino[K-1]][camino[0]] == 1;
        }
        
        for (int v = 0; v < K; v++) {
            if (!visitado[v]) {
                if (pos == 0 || grafo_actual.matriz_adyacencia[camino[pos-1]][v] == 1) {
                    camino[pos] = v;
                    visitado[v] = true;
                    
                    if (backtrack(pos + 1)) {
                        return true;
                    }
                    
                    visitado[v] = false;
                }
            }
        }
        return false;
    }
    
    return backtrack(0);
}

bool tiene_ruta_hamiltoniana() {
    int K = grafo_actual.K;
    if (K < 2) return false;
    
    int camino[K];
    bool visitado[K];
    
    for (int i = 0; i < K; i++) {
        visitado[i] = false;
    }
    
    bool backtrack(int pos) {
        if (pos == K) {
            return true;
        }
        
        for (int v = 0; v < K; v++) {
            if (!visitado[v]) {
                if (pos == 0 || grafo_actual.matriz_adyacencia[camino[pos-1]][v] == 1) {
                    camino[pos] = v;
                    visitado[v] = true;
                    
                    if (backtrack(pos + 1)) {
                        return true;
                    }
                    
                    visitado[v] = false;
                }
            }
        }
        return false;
    }
    
    return backtrack(0);
}

bool encontrar_ciclo_hamiltoniano(int *secuencia, int *longitud) {
    int K = grafo_actual.K;
    if (K < 3) return false;
    
    int camino[K];
    bool visitado[K];
    
    for (int i = 0; i < K; i++) {
        visitado[i] = false;
    }
    
    bool backtrack(int pos) {
        if (pos == K) {
            if (grafo_actual.matriz_adyacencia[camino[K-1]][camino[0]] == 1) {
                for (int i = 0; i < K; i++) {
                    secuencia[i] = camino[i];
                }
                secuencia[K] = camino[0]; // Cerrar el ciclo
                *longitud = K + 1;
                return true;
            }
            return false;
        }
        
        for (int v = 0; v < K; v++) {
            if (!visitado[v]) {
                if (pos == 0 || grafo_actual.matriz_adyacencia[camino[pos-1]][v] == 1) {
                    camino[pos] = v;
                    visitado[v] = true;
                    
                    if (backtrack(pos + 1)) {
                        return true;
                    }
                    
                    visitado[v] = false;
                }
            }
        }
        return false;
    }
    
    return backtrack(0);
}

bool encontrar_ruta_hamiltoniana(int *secuencia, int *longitud) {
    int K = grafo_actual.K;
    if (K < 2) return false;
    
    int camino[K];
    bool visitado[K];
    
    for (int i = 0; i < K; i++) {
        visitado[i] = false;
    }
    
    bool backtrack(int pos) {
        if (pos == K) {
            for (int i = 0; i < K; i++) {
                secuencia[i] = camino[i];
            }
            *longitud = K;
            return true;
        }
        
        for (int v = 0; v < K; v++) {
            if (!visitado[v]) {
                if (pos == 0 || grafo_actual.matriz_adyacencia[camino[pos-1]][v] == 1) {
                    camino[pos] = v;
                    visitado[v] = true;
                    
                    if (backtrack(pos + 1)) {
                        return true;
                    }
                    
                    visitado[v] = false;
                }
            }
        }
        return false;
    }
    
    return backtrack(0);
}

bool es_euleriano() {
    int K = grafo_actual.K;
    
    if (grafo_actual.tipo == NO_DIRIGIDO) {
        bool tiene_aristas = false;
        for (int i = 0; i < K; i++) {
            for (int j = 0; j < K; j++) {
                if (grafo_actual.matriz_adyacencia[i][j] == 1) {
                    tiene_aristas = true;
                    break;
                }
            }
            if (tiene_aristas) break;
        }
        if (!tiene_aristas) return false;
        
        for (int i = 0; i < K; i++) {
            int grado = 0;
            for (int j = 0; j < K; j++) {
                if (grafo_actual.matriz_adyacencia[i][j] == 1) {
                    grado++;
                }
            }
            if (grado % 2 != 0) {
                return false;
            }
        }
        return true;
    } else {
        for (int i = 0; i < K; i++) {
            int grado_entrada = 0;
            int grado_salida = 0;
            
            for (int j = 0; j < K; j++) {
                if (grafo_actual.matriz_adyacencia[j][i] == 1) {
                    grado_entrada++;
                }
                if (grafo_actual.matriz_adyacencia[i][j] == 1) {
                    grado_salida++;
                }
            }
            
            if (grado_entrada != grado_salida) {
                return false;
            }
        }
        return true;
    }
}

bool es_semieuleriano() {
    if (es_euleriano()) return false;
    
    int K = grafo_actual.K;
    
    if (grafo_actual.tipo == NO_DIRIGIDO) {
        int nodos_grado_impar = 0;
        
        for (int i = 0; i < K; i++) {
            int grado = 0;
            for (int j = 0; j < K; j++) {
                if (grafo_actual.matriz_adyacencia[i][j] == 1) {
                    grado++;
                }
            }
            if (grado % 2 != 0) {
                nodos_grado_impar++;
            }
        }
        
        return nodos_grado_impar == 2;
    } else {
        int nodo_inicio = -1, nodo_fin = -1;
        
        for (int i = 0; i < K; i++) {
            int grado_entrada = 0;
            int grado_salida = 0;
            
            for (int j = 0; j < K; j++) {
                if (grafo_actual.matriz_adyacencia[j][i] == 1) {
                    grado_entrada++;
                }
                if (grafo_actual.matriz_adyacencia[i][j] == 1) {
                    grado_salida++;
                }
            }
            
            int diferencia = grado_salida - grado_entrada;
            if (diferencia == 1) {
                if (nodo_inicio == -1) {
                    nodo_inicio = i;
                } else {
                    return false; // Más de un nodo con diferencia +1
                }
            } else if (diferencia == -1) {
                if (nodo_fin == -1) {
                    nodo_fin = i;
                } else {
                    return false; // Más de un nodo con diferencia -1
                }
            } else if (diferencia != 0) {
                return false;
            }
        }
        
        return (nodo_inicio != -1 && nodo_fin != -1);
    }
}

void calcular_grados_no_dirigido(int *grados) {
    int K = grafo_actual.K;
    for (int i = 0; i < K; i++) {
        grados[i] = 0;
        for (int j = 0; j < K; j++) {
            if (grafo_actual.matriz_adyacencia[i][j] == 1) {
                grados[i]++;
            }
        }
    }
}

void calcular_grados_dirigido(int *grados_entrada, int *grados_salida) {
    int K = grafo_actual.K;
    for (int i = 0; i < K; i++) {
        grados_entrada[i] = 0;
        grados_salida[i] = 0;
        for (int j = 0; j < K; j++) {
            if (grafo_actual.matriz_adyacencia[j][i] == 1) {
                grados_entrada[i]++;
            }
            if (grafo_actual.matriz_adyacencia[i][j] == 1) {
                grados_salida[i]++;
            }
        }
    }
}

// Algoritmo de Hierholzer para encontrar ciclo euleriano
int encontrar_ciclo_euleriano_hierholzer(int *secuencia) {
    int K = grafo_actual.K;
    if (!es_euleriano()) return 0;
    
    // Crear copia de la matriz de adyacencia para modificar
    int matriz_copia[MAX_NODOS][MAX_NODOS];
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < K; j++) {
            matriz_copia[i][j] = grafo_actual.matriz_adyacencia[i][j];
        }
    }
    
    // Encontrar vértice inicial (cualquier vértice con aristas)
    int inicio = 0;
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < K; j++) {
            if (matriz_copia[i][j] > 0) {
                inicio = i;
                goto encontrado;
            }
        }
    }
    encontrado:
    
    // Pila para el algoritmo
    int pila[MAX_NODOS * MAX_NODOS];
    int top = 0;
    pila[top++] = inicio;
    
    int resultado[MAX_NODOS * MAX_NODOS];
    int res_len = 0;
    
    while (top > 0) {
        int u = pila[top - 1];
        
        // Buscar arista no usada desde u
        int v = -1;
        for (int i = 0; i < K; i++) {
            if (matriz_copia[u][i] > 0) {
                v = i;
                break;
            }
        }
        
        if (v != -1) {
            // Remover arista
            matriz_copia[u][v]--;
            if (grafo_actual.tipo == NO_DIRIGIDO) {
                matriz_copia[v][u]--;
            }
            pila[top++] = v;
        } else {
            // No hay más aristas, agregar a resultado
            resultado[res_len++] = u;
            top--;
        }
    }
    
    // Copiar resultado invertido (Hierholzer produce resultado al revés)
    for (int i = 0; i < res_len; i++) {
        secuencia[i] = resultado[res_len - 1 - i];
    }
    
    return res_len;
}

// Algoritmo de Fleury para encontrar ciclo euleriano
int encontrar_ciclo_euleriano_fleury(int *secuencia) {
    int K = grafo_actual.K;
    if (!es_euleriano()) return 0;
    
    // Crear copia de la matriz
    int matriz_copia[MAX_NODOS][MAX_NODOS];
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < K; j++) {
            matriz_copia[i][j] = grafo_actual.matriz_adyacencia[i][j];
        }
    }
    
    // Encontrar vértice inicial
    int inicio = 0;
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < K; j++) {
            if (matriz_copia[i][j] > 0) {
                inicio = i;
                goto encontrado_fleury;
            }
        }
    }
    encontrado_fleury:
    
    int resultado[MAX_NODOS * MAX_NODOS];
    int res_len = 0;
    int actual = inicio;
    
    resultado[res_len++] = actual;
    
    while (true) {
        // Contar aristas restantes
        int aristas_restantes = 0;
        for (int i = 0; i < K; i++) {
            for (int j = 0; j < K; j++) {
                aristas_restantes += matriz_copia[i][j];
            }
        }
        if (aristas_restantes == 0) break;
        
        // Buscar siguiente arista (preferir no-puente)
        int siguiente = -1;
        for (int i = 0; i < K; i++) {
            if (matriz_copia[actual][i] > 0) {
                // Verificar si es puente (simplificado)
                siguiente = i;
                // Si solo hay una arista, debe usarla
                int grado_actual = 0;
                for (int j = 0; j < K; j++) {
                    grado_actual += matriz_copia[actual][j];
                }
                if (grado_actual == 1) {
                    break; // Debe usar esta arista
                }
                // Si hay más opciones, usar la primera (simplificación)
                break;
            }
        }
        
        if (siguiente == -1) break;
        
        // Remover arista
        matriz_copia[actual][siguiente]--;
        if (grafo_actual.tipo == NO_DIRIGIDO) {
            matriz_copia[siguiente][actual]--;
        }
        
        actual = siguiente;
        resultado[res_len++] = actual;
    }
    
    for (int i = 0; i < res_len; i++) {
        secuencia[i] = resultado[i];
    }
    
    return res_len;
}

// Algoritmo de Fleury para encontrar ruta euleriana
int encontrar_ruta_euleriana_fleury(int *secuencia) {
    int K = grafo_actual.K;
    if (!es_semieuleriano()) return 0;
    
    // Crear copia de la matriz
    int matriz_copia[MAX_NODOS][MAX_NODOS];
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < K; j++) {
            matriz_copia[i][j] = grafo_actual.matriz_adyacencia[i][j];
        }
    }
    
    // Encontrar vértice inicial (grado impar en no dirigido, o con más salidas en dirigido)
    int inicio = 0;
    if (grafo_actual.tipo == NO_DIRIGIDO) {
        for (int i = 0; i < K; i++) {
            int grado = 0;
            for (int j = 0; j < K; j++) {
                grado += matriz_copia[i][j];
            }
            if (grado % 2 == 1) {
                inicio = i;
                break;
            }
        }
    } else {
        for (int i = 0; i < K; i++) {
            int grado_entrada = 0, grado_salida = 0;
            for (int j = 0; j < K; j++) {
                grado_entrada += matriz_copia[j][i];
                grado_salida += matriz_copia[i][j];
            }
            if (grado_salida > grado_entrada) {
                inicio = i;
                break;
            }
        }
    }
    
    int resultado[MAX_NODOS * MAX_NODOS];
    int res_len = 0;
    int actual = inicio;
    
    resultado[res_len++] = actual;
    
    while (true) {
        // Contar aristas restantes
        int aristas_restantes = 0;
        for (int i = 0; i < K; i++) {
            for (int j = 0; j < K; j++) {
                aristas_restantes += matriz_copia[i][j];
            }
        }
        if (aristas_restantes == 0) break;
        
        // Buscar siguiente arista
        int siguiente = -1;
        for (int i = 0; i < K; i++) {
            if (matriz_copia[actual][i] > 0) {
                siguiente = i;
                int grado_actual = 0;
                for (int j = 0; j < K; j++) {
                    grado_actual += matriz_copia[actual][j];
                }
                if (grado_actual == 1) {
                    break;
                }
                break;
            }
        }
        
        if (siguiente == -1) break;
        
        // Remover arista
        matriz_copia[actual][siguiente]--;
        if (grafo_actual.tipo == NO_DIRIGIDO) {
            matriz_copia[siguiente][actual]--;
        }
        
        actual = siguiente;
        resultado[res_len++] = actual;
    }
    
    for (int i = 0; i < res_len; i++) {
        secuencia[i] = resultado[i];
    }
    
    return res_len;
}

void generar_latex(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window_main),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Error al crear archivo LaTeX");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    int K = grafo_actual.K;
    
    fprintf(f, "\\documentclass[12pt]{article}\n");
    fprintf(f, "\\usepackage[utf8]{inputenc}\n");
    fprintf(f, "\\usepackage[spanish]{babel}\n");
    fprintf(f, "\\usepackage{geometry}\n");
    fprintf(f, "\\usepackage{tikz}\n");
    fprintf(f, "\\usetikzlibrary{positioning, arrows.meta}\n");
    fprintf(f, "\\usepackage{graphicx}\n");
    fprintf(f, "\\usepackage{amsmath}\n");
    fprintf(f, "\\usepackage{xcolor}\n");
    // Configurar búsqueda de imágenes en el directorio actual (compatible con Linux)
    fprintf(f, "\\graphicspath{{./}{./images/}{./imagenes/}}\n");
    fprintf(f, "\\usepackage{pdftexcmds}\n");
    // Definir comando para incluir imágenes opcionales (compatible con Linux)
    // Este comando verifica si la imagen existe antes de intentar incluirla
    fprintf(f, "\\makeatletter\n");
    fprintf(f, "\\newcommand{\\includegraphicsoptional}[2][]{%%\n");
    fprintf(f, "  \\begingroup\n");
    fprintf(f, "  \\pdf@unifilename{#2}%%\n");
    fprintf(f, "  \\edef\\@imgdate{\\pdf@filemoddate{#2}}%%\n");
    fprintf(f, "  \\def\\@imgtest{}%%\n");
    fprintf(f, "  \\ifx\\@imgdate\\@imgtest\n");
    fprintf(f, "    %% Imagen no encontrada - mostrar placeholder\n");
    fprintf(f, "    \\fbox{\\parbox{0.3\\textwidth}{\\centering\\vspace{1.5cm}\\textcolor{gray}{\\textit{[Imagen no disponible]}}\\\\ \\small{(#2)}\\vspace{1.5cm}}}%%\n");
    fprintf(f, "  \\else\n");
    fprintf(f, "    %% Imagen encontrada - incluir normalmente\n");
    fprintf(f, "    \\includegraphics[#1]{#2}%%\n");
    fprintf(f, "  \\fi\n");
    fprintf(f, "  \\endgroup\n");
    fprintf(f, "}\n");
    fprintf(f, "\\makeatother\n");
    fprintf(f, "\\geometry{a4paper, margin=2.5cm}\n");
    fprintf(f, "\\title{Proyecto 4: Hamilton, Euler y Grafos, Parte I}\n");
    fprintf(f, "\\author{Miembros del Grupo:\\\\Ricardo Castro\\\\Juan Carlos Valverde\\\\~\\\\Curso: Analisis de Algoritmos\\\\~\\\\Semestres: II 2025}\n");
    fprintf(f, "\\date{\\today}\n\n");
    fprintf(f, "\\begin{document}\n\n");
    
    fprintf(f, "\\maketitle\n\n");
    fprintf(f, "\\thispagestyle{empty}\n\n");
    fprintf(f, "\\newpage\n\n");
    
    fprintf(f, "\\section{William Rowan Hamilton}\n\n");
    fprintf(f, "\\begin{figure}[h]\n");
    fprintf(f, "\\centering\n");
    fprintf(f, "\\includegraphicsoptional[width=0.3\\textwidth]{hamilton.jpg}\n");
    fprintf(f, "\\caption{William Rowan Hamilton (1805-1865)}\n");
    fprintf(f, "\\end{figure}\n\n");
    
    fprintf(f, "William Rowan Hamilton (1805-1865) fue un matemático, físico y astrónomo irlandés, ");
    fprintf(f, "considerado uno de los científicos más importantes del siglo XIX. Nació en Dublín, ");
    fprintf(f, "Irlanda, y desde muy joven demostró un talento excepcional para las matemáticas y los ");
    fprintf(f, "idiomas. A los 13 años ya dominaba 13 idiomas, incluyendo latín, griego, hebreo, ");
    fprintf(f, "sánscrito y persa.\n\n");
    
    fprintf(f, "Hamilton ingresó al Trinity College de Dublín a los 18 años, donde destacó ");
    fprintf(f, "extraordinariamente. A los 22 años, antes de graduarse, fue nombrado Profesor de ");
    fprintf(f, "Astronomía y Director del Observatorio de Dunsink, posiciones que mantendría durante ");
    fprintf(f, "el resto de su vida. Su trabajo abarcó múltiples áreas de las matemáticas y la física.\n\n");
    
    fprintf(f, "Una de sus contribuciones más significativas fue el desarrollo del \\textbf{álgebra de ");
    fprintf(f, "cuaterniones} en 1843. Los cuaterniones son una extensión de los números complejos a ");
    fprintf(f, "cuatro dimensiones, representados como $q = a + bi + cj + dk$, donde $i$, $j$ y $k$ ");
    fprintf(f, "son unidades imaginarias que satisfacen relaciones específicas. Esta invención tuvo ");
    fprintf(f, "un impacto profundo en la física, especialmente en la mecánica cuántica y la ");
    fprintf(f, "computación gráfica moderna.\n\n");
    
    fprintf(f, "En el campo de la mecánica, Hamilton desarrolló el \\textbf{principio de Hamilton}, ");
    fprintf(f, "también conocido como principio de mínima acción, que reformuló la mecánica clásica ");
    fprintf(f, "en términos de variaciones. Este principio es fundamental en la física teórica y ");
    fprintf(f, "proporciona una base elegante para la mecánica lagrangiana y hamiltoniana.\n\n");
    
    fprintf(f, "En teoría de grafos, Hamilton es conocido por el \\textbf{problema del ciclo ");
    fprintf(f, "hamiltoniano}, que formuló en 1857. El problema consiste en determinar si existe un ");
    fprintf(f, "ciclo en un grafo que visite cada vértice exactamente una vez y regrese al vértice ");
    fprintf(f, "inicial. Este problema, aunque aparentemente simple, resultó ser NP-completo y ha ");
    fprintf(f, "sido objeto de extensa investigación en ciencias de la computación.\n\n");
    
    fprintf(f, "Hamilton también hizo importantes contribuciones a la óptica geométrica, desarrollando ");
    fprintf(f, "una teoría matemática de los rayos de luz que fue precursora de la mecánica cuántica. ");
    fprintf(f, "Su trabajo en óptica estableció conexiones profundas entre la física y las ");
    fprintf(f, "matemáticas.\n\n");
    
    fprintf(f, "A lo largo de su carrera, Hamilton publicó numerosos artículos y mantuvo correspondencia ");
    fprintf(f, "con los principales científicos de su época, incluyendo a John Herschel y Peter Guthrie ");
    fprintf(f, "Tait. Su legado perdura no solo en las matemáticas y la física, sino también en la ");
    fprintf(f, "forma en que concebimos las estructuras algebraicas y los problemas de optimización.\n\n");
    
    fprintf(f, "\\section{Ciclos y Rutas Hamiltonianas}\n\n");
    
    fprintf(f, "\\subsection{Definiciones Fundamentales}\n\n");
    
    fprintf(f, "Un \\textbf{ciclo hamiltoniano} es un ciclo cerrado en un grafo que visita cada vértice ");
    fprintf(f, "exactamente una vez y regresa al vértice inicial. Formalmente, dado un grafo $G = (V, E)$ ");
    fprintf(f, "con $n$ vértices, un ciclo hamiltoniano es una secuencia de vértices $v_1, v_2, \\ldots, v_n, v_1$ ");
    fprintf(f, "tal que:\n\n");
    fprintf(f, "\\begin{itemize}\n");
    fprintf(f, "\\item Cada vértice aparece exactamente una vez en la secuencia (excepto el inicial que ");
    fprintf(f, "aparece al inicio y al final)\n");
    fprintf(f, "\\item Para cada par consecutivo $(v_i, v_{i+1})$ en la secuencia, existe una arista ");
    fprintf(f, "$(v_i, v_{i+1}) \\in E$\n");
    fprintf(f, "\\item Existe una arista $(v_n, v_1) \\in E$ que cierra el ciclo\n");
    fprintf(f, "\\end{itemize}\n\n");
    
    fprintf(f, "Una \\textbf{ruta hamiltoniana} (o camino hamiltoniano) es un camino simple que visita ");
    fprintf(f, "cada vértice exactamente una vez, pero no necesariamente regresa al punto de partida. ");
    fprintf(f, "Es decir, es una secuencia de vértices $v_1, v_2, \\ldots, v_n$ donde cada vértice aparece ");
    fprintf(f, "exactamente una vez y cada par consecutivo está conectado por una arista.\n\n");
    
    fprintf(f, "\\subsection{Importancia y Aplicaciones}\n\n");
    
    fprintf(f, "El problema de determinar si un grafo tiene un ciclo o ruta hamiltoniana es uno de los ");
    fprintf(f, "21 problemas NP-completos originales identificados por Karp en 1972. Esto significa que:\n\n");
    fprintf(f, "\\begin{itemize}\n");
    fprintf(f, "\\item No se conoce un algoritmo eficiente (polinomial) que resuelva el problema para ");
    fprintf(f, "grafos arbitrarios\n");
    fprintf(f, "\\item Si existiera tal algoritmo, se resolverían todos los problemas NP-completos\n");
    fprintf(f, "\\item Los algoritmos actuales tienen complejidad exponencial en el peor caso\n");
    fprintf(f, "\\end{itemize}\n\n");
    
    fprintf(f, "A pesar de su complejidad, los ciclos y rutas hamiltonianas tienen numerosas aplicaciones ");
    fprintf(f, "prácticas:\n\n");
    fprintf(f, "\\begin{itemize}\n");
    fprintf(f, "\\item \\textbf{Problema del viajante (TSP)}: Encontrar la ruta más corta que visite ");
    fprintf(f, "todas las ciudades exactamente una vez\n");
    fprintf(f, "\\item \\textbf{Secuenciación de tareas}: Optimizar el orden de ejecución de tareas ");
    fprintf(f, "con dependencias\n");
    fprintf(f, "\\item \\textbf{Diseño de circuitos}: Encontrar rutas que pasen por todos los puntos ");
    fprintf(f, "de un circuito\n");
    fprintf(f, "\\item \\textbf{Análisis de redes}: Estudiar la conectividad y estructura de redes complejas\n");
    fprintf(f, "\\end{itemize}\n\n");
    
    fprintf(f, "\\subsection{Ejemplos Visuales}\n\n");
    
    fprintf(f, "A continuación se presentan ejemplos gráficos que ilustran estos conceptos:\n\n");
    
    // Ejemplo 1: Ciclo hamiltoniano en un grafo completo
    fprintf(f, "\\subsubsection{Ejemplo 1: Ciclo Hamiltoniano}\n\n");
    fprintf(f, "El siguiente grafo muestra un ejemplo de ciclo hamiltoniano. El ciclo está marcado en ");
    fprintf(f, "rojo y sigue la secuencia: $0 \\rightarrow 1 \\rightarrow 2 \\rightarrow 3 \\rightarrow 0$.\n\n");
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.2]\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (0) at (0,0) {0};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (1) at (2,0) {1};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (2) at (2,2) {2};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (3) at (0,2) {3};\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (1);\n");
    fprintf(f, "\\draw[gray, thin] (1) -- (2);\n");
    fprintf(f, "\\draw[gray, thin] (2) -- (3);\n");
    fprintf(f, "\\draw[gray, thin] (3) -- (0);\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (2);\n");
    fprintf(f, "\\draw[gray, thin] (1) -- (3);\n");
    fprintf(f, "\\draw[red, very thick, ->] (0) to[bend left=15] (1);\n");
    fprintf(f, "\\draw[red, very thick, ->] (1) to[bend left=15] (2);\n");
    fprintf(f, "\\draw[red, very thick, ->] (2) to[bend left=15] (3);\n");
    fprintf(f, "\\draw[red, very thick, ->] (3) to[bend left=15] (0);\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    // Ejemplo 2: Ruta hamiltoniana
    fprintf(f, "\\subsubsection{Ejemplo 2: Ruta Hamiltoniana}\n\n");
    fprintf(f, "El siguiente grafo muestra una ruta hamiltoniana (no un ciclo, ya que no regresa al ");
    fprintf(f, "vértice inicial). La ruta está marcada en verde y sigue: $0 \\rightarrow 1 \\rightarrow 2 \\rightarrow 3$.\n\n");
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.2]\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (0) at (0,0) {0};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (1) at (2,0) {1};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (2) at (2,2) {2};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (3) at (0,2) {3};\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (1);\n");
    fprintf(f, "\\draw[gray, thin] (1) -- (2);\n");
    fprintf(f, "\\draw[gray, thin] (2) -- (3);\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (2);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (0) to[bend left=15] (1);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (1) to[bend left=15] (2);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (2) to[bend left=15] (3);\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    // Ejemplo 3: Grafo sin ciclo hamiltoniano
    fprintf(f, "\\subsubsection{Ejemplo 3: Grafo sin Ciclo Hamiltoniano}\n\n");
    fprintf(f, "No todos los grafos tienen ciclos hamiltonianos. El siguiente grafo (un árbol) no tiene ");
    fprintf(f, "ciclo hamiltoniano porque es acíclico, pero sí tiene una ruta hamiltoniana.\n\n");
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.2]\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (0) at (1,0) {0};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (1) at (0,1.5) {1};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (2) at (2,1.5) {2};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (3) at (0,3) {3};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (4) at (2,3) {4};\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (1);\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (2);\n");
    fprintf(f, "\\draw[gray, thin] (1) -- (3);\n");
    fprintf(f, "\\draw[gray, thin] (2) -- (4);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (3) to[bend right=10] (1);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (1) to[bend right=10] (0);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (0) to[bend right=10] (2);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (2) to[bend right=10] (4);\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    fprintf(f, "\\subsection{Algoritmos y Complejidad}\n\n");
    
    fprintf(f, "En este proyecto, se utiliza un algoritmo de \\textbf{backtracking} (vuelta atrás) ");
    fprintf(f, "para determinar si existe al menos un ciclo o ruta hamiltoniana en el grafo. El algoritmo ");
    fprintf(f, "explora sistemáticamente todas las posibles rutas, retrocediendo cuando una ruta parcial ");
    fprintf(f, "no puede completarse.\n\n");
    
    fprintf(f, "La complejidad temporal del algoritmo de backtracking para este problema es $O(n!)$ en el ");
    fprintf(f, "peor caso, donde $n$ es el número de vértices. Esto se debe a que, en el peor escenario, ");
    fprintf(f, "debe explorar todas las permutaciones posibles de los vértices.\n\n");
    
    fprintf(f, "Aunque el algoritmo implementado determina la \\textit{existencia} de un ciclo o ruta ");
    fprintf(f, "hamiltoniana, no encuentra la solución específica. Para encontrar la solución completa, ");
    fprintf(f, "sería necesario modificar el algoritmo para almacenar y retornar la secuencia de vértices ");
    fprintf(f, "que forma el ciclo o ruta.\n\n");
    
    fprintf(f, "\\section{Leonhard Euler}\n\n");
    fprintf(f, "\\begin{figure}[h]\n");
    fprintf(f, "\\centering\n");
    fprintf(f, "\\includegraphicsoptional[width=0.3\\textwidth]{euler.jpg}\n");
    fprintf(f, "\\caption{Leonhard Euler (1707-1783)}\n");
    fprintf(f, "\\end{figure}\n\n");
    
    fprintf(f, "Leonhard Euler (1707-1783) fue un matemático y físico suizo considerado uno de los ");
    fprintf(f, "matemáticos más prolíficos e influyentes de la historia. Nació en Basilea, Suiza, y ");
    fprintf(f, "desde muy joven mostró un talento excepcional para las matemáticas. Su padre, Paul ");
    fprintf(f, "Euler, era pastor calvinista y quería que su hijo siguiera sus pasos, pero el joven ");
    fprintf(f, "Leonhard estaba destinado a convertirse en una de las mentes más brillantes de la ciencia.\n\n");
    
    fprintf(f, "Euler estudió en la Universidad de Basilea bajo la tutela de Johann Bernoulli, uno de ");
    fprintf(f, "los matemáticos más destacados de su época. A los 19 años, Euler ya había completado ");
    fprintf(f, "su maestría y comenzaba a publicar trabajos matemáticos. En 1727, a los 20 años, fue ");
    fprintf(f, "invitado a unirse a la Academia de Ciencias de San Petersburgo, donde permanecería hasta ");
    fprintf(f, "1741, y luego regresaría en 1766 hasta su muerte.\n\n");
    
    fprintf(f, "La productividad de Euler fue extraordinaria. Publicó más de 800 artículos y libros ");
    fprintf(f, "durante su vida, y su obra completa, que aún se está compilando, se estima que ");
    fprintf(f, "comprenderá más de 80 volúmenes. Sus contribuciones abarcan prácticamente todas las ");
    fprintf(f, "áreas de las matemáticas: análisis matemático, teoría de números, geometría, álgebra, ");
    fprintf(f, "mecánica, óptica, astronomía y música.\n\n");
    
    fprintf(f, "Una de sus contribuciones más fundamentales fue el desarrollo y sistematización del ");
    fprintf(f, "\\textbf{cálculo infinitesimal}. Euler introdujo la notación matemática moderna que ");
    fprintf(f, "usamos hoy en día, incluyendo el símbolo $e$ para la base del logaritmo natural ");
    fprintf(f, "(aproximadamente 2.71828), el símbolo $i$ para la unidad imaginaria, y la notación ");
    fprintf(f, "$f(x)$ para funciones. También estableció la famosa identidad $e^{i\\pi} + 1 = 0$, ");
    fprintf(f, "conocida como la identidad de Euler, que conecta cinco de los números más importantes ");
    fprintf(f, "en matemáticas.\n\n");
    
    fprintf(f, "En 1736, Euler resolvió el \\textbf{problema de los puentes de Königsberg}, que se ");
    fprintf(f, "considera el origen de la teoría de grafos. El problema consistía en determinar si ");
    fprintf(f, "era posible cruzar los siete puentes de la ciudad de Königsberg (hoy Kaliningrado) ");
    fprintf(f, "exactamente una vez y regresar al punto de partida. Euler demostró que esto era ");
    fprintf(f, "imposible, sentando las bases para lo que hoy conocemos como grafos eulerianos.\n\n");
    
    fprintf(f, "Euler hizo contribuciones fundamentales a la \\textbf{teoría de números}, incluyendo ");
    fprintf(f, "el teorema de Euler en aritmética modular, que generaliza el pequeño teorema de Fermat. ");
    fprintf(f, "También trabajó extensamente en la función zeta de Riemann (aunque no con ese nombre), ");
    fprintf(f, "estableciendo la relación entre los números primos y los números naturales.\n\n");
    
    fprintf(f, "En \\textbf{mecánica}, Euler desarrolló las ecuaciones de movimiento de Euler para ");
    fprintf(f, "cuerpos rígidos y contribuyó significativamente a la mecánica de fluidos. Sus trabajos ");
    fprintf(f, "en óptica y astronomía también fueron pioneros, incluyendo cálculos precisos de órbitas ");
    fprintf(f, "planetarias y el desarrollo de la teoría de la refracción de la luz.\n\n");
    
    fprintf(f, "A pesar de perder la visión de un ojo en 1735 y quedarse completamente ciego en 1766, ");
    fprintf(f, "Euler continuó trabajando con una productividad asombrosa. Su memoria prodigiosa le ");
    fprintf(f, "permitía realizar cálculos complejos mentalmente, y dictaba sus trabajos a sus hijos ");
    fprintf(f, "y asistentes. De hecho, algunos de sus trabajos más importantes fueron producidos ");
    fprintf(f, "durante su ceguera total.\n\n");
    
    fprintf(f, "El legado de Euler es inmenso. Muchos conceptos, teoremas y fórmulas llevan su nombre: ");
    fprintf(f, "el número de Euler ($e$), la fórmula de Euler, el método de Euler para ecuaciones ");
    fprintf(f, "diferenciales, los ángulos de Euler, la función phi de Euler, y muchos más. Su ");
    fprintf(f, "influencia se extiende no solo a las matemáticas puras, sino también a la física, la ");
    fprintf(f, "ingeniería y la computación moderna.\n\n");
    
    fprintf(f, "Euler murió en San Petersburgo en 1783, dejando un legado que continúa inspirando a ");
    fprintf(f, "matemáticos y científicos hasta el día de hoy. Su enfoque sistemático, su notación ");
    fprintf(f, "clara y su capacidad para encontrar conexiones profundas entre diferentes áreas del ");
    fprintf(f, "conocimiento lo convierten en uno de los pilares fundamentales de las matemáticas modernas.\n\n");
    
    fprintf(f, "\\section{Ciclos y Rutas Eulerianas}\n\n");
    
    fprintf(f, "\\subsection{El Problema de los Puentes de Königsberg}\n\n");
    
    fprintf(f, "El origen de los ciclos eulerianos se remonta al famoso \\textbf{problema de los puentes ");
    fprintf(f, "de Königsberg}, resuelto por Euler en 1736. La ciudad de Königsberg (hoy Kaliningrado) ");
    fprintf(f, "estaba dividida por el río Pregel en cuatro regiones conectadas por siete puentes. ");
    fprintf(f, "El problema consistía en determinar si era posible dar un paseo que cruzara cada puente ");
    fprintf(f, "exactamente una vez y regresar al punto de partida.\n\n");
    
    fprintf(f, "Euler modeló este problema como un grafo, donde cada región era un vértice y cada puente ");
    fprintf(f, "era una arista. Demostró que tal recorrido era imposible, estableciendo así los ");
    fprintf(f, "fundamentos de la teoría de grafos y los grafos eulerianos.\n\n");
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.0]\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.6cm] (A) at (0,0) {A};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.6cm] (B) at (2,0) {B};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.6cm] (C) at (1,1.5) {C};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.6cm] (D) at (1,-1.5) {D};\n");
    fprintf(f, "\\draw[thick] (A) -- (C);\n");
    fprintf(f, "\\draw[thick] (A) -- (C);\n");
    fprintf(f, "\\draw[thick] (A) -- (D);\n");
    fprintf(f, "\\draw[thick] (B) -- (C);\n");
    fprintf(f, "\\draw[thick] (B) -- (C);\n");
    fprintf(f, "\\draw[thick] (B) -- (D);\n");
    fprintf(f, "\\draw[thick] (C) -- (D);\n");
    fprintf(f, "\\node[below] at (1,-2.2) {Grafo de los puentes de Königsberg};\n");
    fprintf(f, "\\node[below] at (1,-2.6) {Grados: A=3, B=3, C=5, D=3 (todos impares)};\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    fprintf(f, "Como todos los vértices tienen grado impar, el grafo no puede tener un ciclo euleriano. ");
    fprintf(f, "Este fue el primer resultado en teoría de grafos.\n\n");
    
    fprintf(f, "\\subsection{Definiciones Fundamentales}\n\n");
    
    fprintf(f, "Un \\textbf{ciclo euleriano} es un ciclo cerrado en un grafo que recorre cada arista ");
    fprintf(f, "exactamente una vez y regresa al vértice inicial. Formalmente, dado un grafo $G = (V, E)$ ");
    fprintf(f, "con $m$ aristas, un ciclo euleriano es una secuencia de vértices $v_1, v_2, \\ldots, v_k, v_1$ ");
    fprintf(f, "tal que:\n\n");
    fprintf(f, "\\begin{itemize}\n");
    fprintf(f, "\\item Cada arista del grafo aparece exactamente una vez en el ciclo\n");
    fprintf(f, "\\item El ciclo comienza y termina en el mismo vértice\n");
    fprintf(f, "\\item Cada par consecutivo de vértices está conectado por una arista\n");
    fprintf(f, "\\end{itemize}\n\n");
    
    fprintf(f, "Un \\textbf{camino euleriano} (o ruta euleriana) es un camino que recorre cada arista ");
    fprintf(f, "exactamente una vez, pero no necesariamente regresa al punto de partida. Es decir, es ");
    fprintf(f, "una secuencia de vértices $v_1, v_2, \\ldots, v_k$ donde cada arista aparece exactamente ");
    fprintf(f, "una vez.\n\n");
    
    fprintf(f, "Un grafo es \\textbf{euleriano} si tiene un ciclo euleriano. Un grafo es ");
    fprintf(f, "\\textbf{semieuleriano} si tiene un camino euleriano pero no un ciclo euleriano.\n\n");
    
    fprintf(f, "\\subsection{Teorema de Euler}\n\n");
    
    fprintf(f, "Euler estableció condiciones necesarias y suficientes para la existencia de ciclos y ");
    fprintf(f, "caminos eulerianos:\n\n");
    
    fprintf(f, "\\textbf{Teorema (Euler, 1736):} Para un grafo no dirigido conexo $G$:\n\n");
    fprintf(f, "\\begin{itemize}\n");
    fprintf(f, "\\item $G$ tiene un ciclo euleriano si y solo si todos los vértices tienen grado par\n");
    fprintf(f, "\\item $G$ tiene un camino euleriano (pero no un ciclo) si y solo si tiene exactamente ");
    fprintf(f, "dos vértices de grado impar. Estos vértices serán el inicio y el final del camino\n");
    fprintf(f, "\\end{itemize}\n\n");
    
    fprintf(f, "Para grafos dirigidos:\n\n");
    fprintf(f, "\\begin{itemize}\n");
    fprintf(f, "\\item Un grafo dirigido tiene un ciclo euleriano si y solo si es fuertemente conexo ");
    fprintf(f, "y cada vértice tiene el mismo grado de entrada que de salida\n");
    fprintf(f, "\\item Un grafo dirigido tiene un camino euleriano si tiene exactamente un vértice ");
    fprintf(f, "con $\\deg_{salida} - \\deg_{entrada} = 1$ (inicio), exactamente un vértice con ");
    fprintf(f, "$\\deg_{entrada} - \\deg_{salida} = 1$ (fin), y todos los demás tienen grados iguales\n");
    fprintf(f, "\\end{itemize}\n\n");
    
    fprintf(f, "\\subsection{Ejemplos Visuales}\n\n");
    
    fprintf(f, "A continuación se presentan ejemplos gráficos que ilustran estos conceptos:\n\n");
    
    // Ejemplo 1: Ciclo euleriano
    fprintf(f, "\\subsubsection{Ejemplo 1: Grafo Euleriano (Ciclo Euleriano)}\n\n");
    fprintf(f, "El siguiente grafo es euleriano porque todos los vértices tienen grado par (2 o 4). ");
    fprintf(f, "El ciclo euleriano está marcado en rojo y recorre todas las aristas exactamente una vez.\n\n");
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.2]\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (0) at (0,0) {0};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (1) at (2,0) {1};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (2) at (2,2) {2};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (3) at (0,2) {3};\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (1);\n");
    fprintf(f, "\\draw[gray, thin] (1) -- (2);\n");
    fprintf(f, "\\draw[gray, thin] (2) -- (3);\n");
    fprintf(f, "\\draw[gray, thin] (3) -- (0);\n");
    fprintf(f, "\\draw[red, very thick, ->] (0) to[bend left=15] node[above] {1} (1);\n");
    fprintf(f, "\\draw[red, very thick, ->] (1) to[bend left=15] node[right] {2} (2);\n");
    fprintf(f, "\\draw[red, very thick, ->] (2) to[bend left=15] node[below] {3} (3);\n");
    fprintf(f, "\\draw[red, very thick, ->] (3) to[bend left=15] node[left] {4} (0);\n");
    fprintf(f, "\\node[below] at (1,-0.8) {Ciclo: $0 \\rightarrow 1 \\rightarrow 2 \\rightarrow 3 \\rightarrow 0$};\n");
    fprintf(f, "\\node[below] at (1,-1.2) {Grados: todos pares (2)};\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    // Ejemplo 2: Camino euleriano (semieuleriano)
    fprintf(f, "\\subsubsection{Ejemplo 2: Grafo Semieuleriano (Camino Euleriano)}\n\n");
    fprintf(f, "El siguiente grafo es semieuleriano porque tiene exactamente dos vértices de grado impar ");
    fprintf(f, "(vértices 0 y 3, ambos con grado 3). El camino euleriano está marcado en verde y ");
    fprintf(f, "comienza en el vértice 0 y termina en el vértice 3.\n\n");
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.2]\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (0) at (0,0) {0};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (1) at (2,0) {1};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (2) at (2,2) {2};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (3) at (0,2) {3};\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (1);\n");
    fprintf(f, "\\draw[gray, thin] (1) -- (2);\n");
    fprintf(f, "\\draw[gray, thin] (2) -- (3);\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (3);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (0) to[bend left=15] node[above] {1} (1);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (1) to[bend left=15] node[right] {2} (2);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (2) to[bend left=15] node[above] {3} (3);\n");
    fprintf(f, "\\draw[green!70!black, very thick, ->] (3) to[bend left=15] node[left] {4} (0);\n");
    fprintf(f, "\\node[below] at (1,-0.8) {Camino: $0 \\rightarrow 1 \\rightarrow 2 \\rightarrow 3 \\rightarrow 0$};\n");
    fprintf(f, "\\node[below] at (1,-1.2) {Grados: 0=3, 1=2, 2=2, 3=3 (dos impares)};\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    // Ejemplo 3: Grafo no euleriano
    fprintf(f, "\\subsubsection{Ejemplo 3: Grafo No Euleriano}\n\n");
    fprintf(f, "El siguiente grafo no es euleriano ni semieuleriano porque tiene más de dos vértices ");
    fprintf(f, "de grado impar (vértices 0, 1, 2 y 3, todos con grado 3). Por lo tanto, no existe ");
    fprintf(f, "ningún camino que recorra todas las aristas exactamente una vez.\n\n");
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.2]\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (0) at (0,0) {0};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (1) at (2,0) {1};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (2) at (2,2) {2};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (3) at (0,2) {3};\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (1);\n");
    fprintf(f, "\\draw[gray, thin] (1) -- (2);\n");
    fprintf(f, "\\draw[gray, thin] (2) -- (3);\n");
    fprintf(f, "\\draw[gray, thin] (3) -- (0);\n");
    fprintf(f, "\\draw[gray, thin] (0) -- (2);\n");
    fprintf(f, "\\draw[gray, thin] (1) -- (3);\n");
    fprintf(f, "\\node[below] at (1,-0.8) {Grados: todos impares (3)};\n");
    fprintf(f, "\\node[below] at (1,-1.2) {No es euleriano ni semieuleriano};\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    // Ejemplo 4: Grafo dirigido euleriano
    fprintf(f, "\\subsubsection{Ejemplo 4: Grafo Dirigido Euleriano}\n\n");
    fprintf(f, "En grafos dirigidos, un ciclo euleriano requiere que cada vértice tenga el mismo grado ");
    fprintf(f, "de entrada que de salida. El siguiente ejemplo muestra un grafo dirigido euleriano.\n\n");
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=1.2, >=Stealth]\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (0) at (0,0) {0};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (1) at (2,0) {1};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (2) at (2,2) {2};\n");
    fprintf(f, "\\node[circle, draw=black, fill=blue!30, minimum size=0.8cm] (3) at (0,2) {3};\n");
    fprintf(f, "\\draw[gray, thin, ->] (0) -- (1);\n");
    fprintf(f, "\\draw[gray, thin, ->] (1) -- (2);\n");
    fprintf(f, "\\draw[gray, thin, ->] (2) -- (3);\n");
    fprintf(f, "\\draw[gray, thin, ->] (3) -- (0);\n");
    fprintf(f, "\\draw[red, very thick, ->] (0) to[bend left=15] node[above] {1} (1);\n");
    fprintf(f, "\\draw[red, very thick, ->] (1) to[bend left=15] node[right] {2} (2);\n");
    fprintf(f, "\\draw[red, very thick, ->] (2) to[bend left=15] node[below] {3} (3);\n");
    fprintf(f, "\\draw[red, very thick, ->] (3) to[bend left=15] node[left] {4} (0);\n");
    fprintf(f, "\\node[below] at (1,-0.8) {Ciclo: $0 \\rightarrow 1 \\rightarrow 2 \\rightarrow 3 \\rightarrow 0$};\n");
    fprintf(f, "\\node[below] at (1,-1.2) {Grados: entrada = salida para todos (1)};\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    fprintf(f, "\\subsection{Aplicaciones Prácticas}\n\n");
    
    fprintf(f, "Los ciclos y caminos eulerianos tienen numerosas aplicaciones en la vida real:\n\n");
    
    fprintf(f, "\\begin{itemize}\n");
    fprintf(f, "\\item \\textbf{Recolección de basura}: Optimizar rutas para que los camiones pasen ");
    fprintf(f, "por todas las calles exactamente una vez\n");
    fprintf(f, "\\item \\textbf{Inspección de redes}: Verificar todas las conexiones de una red (eléctrica, ");
    fprintf(f, "de agua, de datos) de manera eficiente\n");
    fprintf(f, "\\item \\textbf{Impresión de circuitos}: Encontrar rutas que pasen por todas las líneas ");
    fprintf(f, "de un circuito impreso sin repetición\n");
    fprintf(f, "\\item \\textbf{Postal y entrega}: Diseñar rutas de entrega que cubran todas las ");
    fprintf(f, "calles sin duplicar esfuerzo\n");
    fprintf(f, "\\item \\textbf{Análisis de ADN}: Reconstruir secuencias genéticas a partir de fragmentos\n");
    fprintf(f, "\\end{itemize}\n\n");
    
    fprintf(f, "\\section{Grafo Original}\n\n");
    
    int min_x = grafo_actual.posiciones[0].x;
    int max_x = grafo_actual.posiciones[0].x;
    int min_y = grafo_actual.posiciones[0].y;
    int max_y = grafo_actual.posiciones[0].y;
    
    for (int i = 1; i < K; i++) {
        if (grafo_actual.posiciones[i].x < min_x) min_x = grafo_actual.posiciones[i].x;
        if (grafo_actual.posiciones[i].x > max_x) max_x = grafo_actual.posiciones[i].x;
        if (grafo_actual.posiciones[i].y < min_y) min_y = grafo_actual.posiciones[i].y;
        if (grafo_actual.posiciones[i].y > max_y) max_y = grafo_actual.posiciones[i].y;
    }
    
    double ancho = (max_x - min_x > 0) ? (max_x - min_x) : 1.0;
    double alto = (max_y - min_y > 0) ? (max_y - min_y) : 1.0;
    double escala = 12.0 / (ancho > alto ? ancho : alto);
    
    fprintf(f, "\\begin{center}\n");
    fprintf(f, "\\begin{tikzpicture}[scale=%.2f]\n", escala);
    
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < K; j++) {
            if (grafo_actual.matriz_adyacencia[i][j] == 1) {
                double x1 = grafo_actual.posiciones[i].x - min_x;
                double y1 = grafo_actual.posiciones[i].y - min_y;
                double x2 = grafo_actual.posiciones[j].x - min_x;
                double y2 = grafo_actual.posiciones[j].y - min_y;
                
                if (grafo_actual.tipo == DIRIGIDO) {
                    fprintf(f, "\\draw[->, thick] (%.2f,%.2f) -- (%.2f,%.2f);\n", x1, y1, x2, y2);
                } else {
                    if (i < j) {
                        fprintf(f, "\\draw[thick] (%.2f,%.2f) -- (%.2f,%.2f);\n", x1, y1, x2, y2);
                    }
                }
            }
        }
    }
    
    int grados[MAX_NODOS];
    int grados_entrada[MAX_NODOS];
    int grados_salida[MAX_NODOS];
    
    if (grafo_actual.tipo == NO_DIRIGIDO) {
        calcular_grados_no_dirigido(grados);
    } else {
        calcular_grados_dirigido(grados_entrada, grados_salida);
    }
    
    for (int i = 0; i < K; i++) {
        double x = grafo_actual.posiciones[i].x - min_x;
        double y = grafo_actual.posiciones[i].y - min_y;
        
        if (grafo_actual.tipo == NO_DIRIGIDO) {
            const char *color = (grados[i] % 2 == 0) ? "white" : "black!80";
            const char *text_color = (grados[i] % 2 == 0) ? "black" : "white";
            fprintf(f, "\\node[circle, draw=black, fill=%s, minimum size=0.8cm, font=\\scriptsize, text=%s] (n%d) at (%.2f,%.2f) {%d};\n",
                color, text_color, i, x, y, i);
        } else {
            const char *color;
            int ent_par = (grados_entrada[i] % 2 == 0) ? 1 : 0;
            int sal_par = (grados_salida[i] % 2 == 0) ? 1 : 0;
            
            if (ent_par && sal_par) color = "blue!30";
            else if (ent_par && !sal_par) color = "green!30";
            else if (!ent_par && sal_par) color = "yellow!30";
            else color = "red!30";
            
            fprintf(f, "\\node[circle, draw=black, fill=%s, minimum size=0.8cm, font=\\scriptsize] (n%d) at (%.2f,%.2f) {%d};\n",
                color, i, x, y, i);
        }
    }
    
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{center}\n\n");
    
    fprintf(f, "\\subsection{Leyenda de Colores}\n\n");
    if (grafo_actual.tipo == NO_DIRIGIDO) {
        fprintf(f, "\\begin{itemize}\n");
        fprintf(f, "\\item \\fcolorbox{black}{black!80}{\\rule{0.5cm}{0.5cm}} Nodos de grado impar\n");
        fprintf(f, "\\item \\fcolorbox{black}{white}{\\rule{0.5cm}{0.5cm}} Nodos de grado par\n");
        fprintf(f, "\\end{itemize}\n\n");
    } else {
        fprintf(f, "\\begin{itemize}\n");
        fprintf(f, "\\item \\fcolorbox{black}{blue!30}{\\rule{0.5cm}{0.5cm}} Grado de entrada par, grado de salida par\n");
        fprintf(f, "\\item \\fcolorbox{black}{green!30}{\\rule{0.5cm}{0.5cm}} Grado de entrada par, grado de salida impar\n");
        fprintf(f, "\\item \\fcolorbox{black}{yellow!30}{\\rule{0.5cm}{0.5cm}} Grado de entrada impar, grado de salida par\n");
        fprintf(f, "\\item \\fcolorbox{black}{red!30}{\\rule{0.5cm}{0.5cm}} Grado de entrada impar, grado de salida impar\n");
        fprintf(f, "\\end{itemize}\n\n");
    }
    
    fprintf(f, "\\section{Propiedades del Grafo}\n\n");
    
    bool tiene_ciclo = tiene_ciclo_hamiltoniano();
    bool tiene_ruta = tiene_ruta_hamiltoniana();
    bool euler = es_euleriano();
    bool semi_euler = es_semieuleriano();
    
    fprintf(f, "\\subsection{Ciclos y Rutas Hamiltonianas}\n\n");
    
    fprintf(f, "Para determinar la existencia de ciclos y rutas hamiltonianas en este grafo, se ha ");
    fprintf(f, "utilizado un algoritmo de backtracking que explora sistemáticamente todas las posibles ");
    fprintf(f, "secuencias de vértices. El algoritmo verifica si existe al menos una permutación de los ");
    fprintf(f, "vértices que forme un ciclo o ruta válida según las aristas presentes en el grafo.\n\n");
    
    if (tiene_ciclo) {
        fprintf(f, "\\textbf{Resultado: El grafo contiene al menos un ciclo hamiltoniano.}\n\n");
        fprintf(f, "Esto significa que existe al menos una secuencia de vértices $v_1, v_2, \\ldots, v_n, v_1$ ");
        fprintf(f, "tal que:\n\n");
        fprintf(f, "\\begin{itemize}\n");
        fprintf(f, "\\item Cada vértice del grafo aparece exactamente una vez en la secuencia (excepto ");
        fprintf(f, "el vértice inicial que aparece al inicio y al final)\n");
        fprintf(f, "\\item Cada par consecutivo de vértices en la secuencia está conectado por una arista\n");
        fprintf(f, "\\item El último vértice está conectado al primero, cerrando el ciclo\n");
        fprintf(f, "\\end{itemize}\n\n");
        fprintf(f, "La existencia de un ciclo hamiltoniano indica que es posible visitar todos los ");
        fprintf(f, "vértices del grafo exactamente una vez y regresar al punto de partida, siguiendo ");
        fprintf(f, "únicamente las aristas existentes. Esta propiedad es de gran importancia en problemas ");
        fprintf(f, "de optimización como el problema del viajante (TSP) y en el diseño de circuitos.\n\n");
    } else {
        fprintf(f, "\\textbf{Resultado: El grafo no contiene un ciclo hamiltoniano.}\n\n");
        fprintf(f, "Esto significa que no existe ninguna secuencia de vértices que forme un ciclo ");
        fprintf(f, "cerrado visitando cada vértice exactamente una vez. Las posibles razones para esto ");
        fprintf(f, "pueden incluir:\n\n");
        fprintf(f, "\\begin{itemize}\n");
        fprintf(f, "\\item El grafo no es suficientemente denso (no hay suficientes aristas para conectar ");
        fprintf(f, "todos los vértices en un ciclo)\n");
        fprintf(f, "\\item Existen vértices de grado muy bajo que restringen las posibles rutas\n");
        fprintf(f, "\\item La estructura del grafo impide la formación de un ciclo que visite todos ");
        fprintf(f, "los vértices\n");
        fprintf(f, "\\end{itemize}\n\n");
        fprintf(f, "Es importante notar que la ausencia de un ciclo hamiltoniano no implica necesariamente ");
        fprintf(f, "la ausencia de una ruta hamiltoniana, ya que una ruta no requiere regresar al ");
        fprintf(f, "vértice inicial.\n\n");
    }
    
    if (tiene_ruta) {
        fprintf(f, "\\textbf{Resultado: El grafo contiene al menos una ruta hamiltoniana.}\n\n");
        fprintf(f, "Esto significa que existe al menos una secuencia de vértices $v_1, v_2, \\ldots, v_n$ ");
        fprintf(f, "tal que:\n\n");
        fprintf(f, "\\begin{itemize}\n");
        fprintf(f, "\\item Cada vértice del grafo aparece exactamente una vez en la secuencia\n");
        fprintf(f, "\\item Cada par consecutivo de vértices en la secuencia está conectado por una arista\n");
        fprintf(f, "\\item El camino no necesariamente regresa al vértice inicial\n");
        fprintf(f, "\\end{itemize}\n\n");
        fprintf(f, "La existencia de una ruta hamiltoniana indica que es posible visitar todos los ");
        fprintf(f, "vértices del grafo exactamente una vez, aunque no se regrese al punto de partida. ");
        fprintf(f, "Esta propiedad es útil en problemas de secuenciación, donde se necesita un orden ");
        fprintf(f, "específico de elementos sin repetición.\n\n");
        if (!tiene_ciclo) {
            fprintf(f, "Nótese que aunque este grafo tiene una ruta hamiltoniana, no tiene un ciclo ");
            fprintf(f, "hamiltoniano. Esto significa que cualquier ruta que visite todos los vértices ");
            fprintf(f, "debe comenzar y terminar en vértices diferentes, y no puede cerrarse formando ");
            fprintf(f, "un ciclo.\n\n");
        }
    } else {
        fprintf(f, "\\textbf{Resultado: El grafo no contiene una ruta hamiltoniana.}\n\n");
        fprintf(f, "Esto significa que no existe ninguna secuencia de vértices que visite cada vértice ");
        fprintf(f, "exactamente una vez siguiendo las aristas del grafo. Si el grafo tampoco tiene un ");
        fprintf(f, "ciclo hamiltoniano, entonces no es posible visitar todos los vértices en una sola ");
        fprintf(f, "trayectoria sin repetir vértices.\n\n");
        fprintf(f, "Las posibles razones para la ausencia de una ruta hamiltoniana incluyen:\n\n");
        fprintf(f, "\\begin{itemize}\n");
        fprintf(f, "\\item El grafo tiene vértices aislados o componentes desconectados\n");
        fprintf(f, "\\item La estructura del grafo es demasiado restrictiva (por ejemplo, tiene forma ");
        fprintf(f, "de árbol con muchas hojas)\n");
        fprintf(f, "\\item Existen vértices que actúan como \"cuellos de botella\" que impiden el ");
        fprintf(f, "recorrido completo\n");
        fprintf(f, "\\end{itemize}\n\n");
    }
    
    fprintf(f, "\\subsection{Propiedades Eulerianas}\n\n");
    
    fprintf(f, "Para determinar las propiedades eulerianas del grafo, se ha analizado la paridad de ");
    fprintf(f, "los grados de los vértices. Según el teorema de Euler (1736), las condiciones para ");
    fprintf(f, "la existencia de ciclos y caminos eulerianos dependen directamente de los grados de ");
    fprintf(f, "los vértices, lo que hace que este problema sea computacionalmente más simple que el ");
    fprintf(f, "problema hamiltoniano.\n\n");
    
    if (euler) {
        fprintf(f, "\\textbf{Resultado: El grafo es euleriano.}\n\n");
        fprintf(f, "Esto significa que el grafo contiene un \\textbf{ciclo euleriano}, es decir, un ");
        fprintf(f, "ciclo cerrado que recorre cada arista del grafo exactamente una vez y regresa al ");
        fprintf(f, "vértice inicial. Esta es una propiedad muy deseable en aplicaciones prácticas como ");
        fprintf(f, "la optimización de rutas de recolección, inspección de redes y diseño de circuitos.\n\n");
        
        if (grafo_actual.tipo == NO_DIRIGIDO) {
            fprintf(f, "\\textbf{Análisis para grafo no dirigido:}\n\n");
            fprintf(f, "Para que un grafo no dirigido sea euleriano, según el teorema de Euler, deben ");
            fprintf(f, "cumplirse dos condiciones:\n\n");
            fprintf(f, "\\begin{enumerate}\n");
            fprintf(f, "\\item El grafo debe ser \\textbf{conexo}: todos los vértices deben estar ");
            fprintf(f, "conectados entre sí, de manera que exista un camino entre cualquier par de ");
            fprintf(f, "vértices.\n");
            fprintf(f, "\\item Todos los vértices deben tener \\textbf{grado par}: cada vértice debe ");
            fprintf(f, "estar conectado a un número par de aristas.\n");
            fprintf(f, "\\end{enumerate}\n\n");
            fprintf(f, "En este grafo, se ha verificado que ambas condiciones se cumplen. La condición ");
            fprintf(f, "de grados pares es necesaria porque, en un ciclo euleriano, cada vez que se ");
            fprintf(f, "entra a un vértice por una arista, se debe salir por otra arista diferente. ");
            fprintf(f, "Por lo tanto, cada vértice debe tener un número par de aristas incidentes.\n\n");
        } else {
            fprintf(f, "\\textbf{Análisis para grafo dirigido:}\n\n");
            fprintf(f, "Para que un grafo dirigido sea euleriano, según el teorema de Euler para grafos ");
            fprintf(f, "dirigidos, deben cumplirse dos condiciones:\n\n");
            fprintf(f, "\\begin{enumerate}\n");
            fprintf(f, "\\item El grafo debe ser \\textbf{fuertemente conexo}: debe existir un camino ");
            fprintf(f, "dirigido desde cualquier vértice hacia cualquier otro vértice.\n");
            fprintf(f, "\\item Cada vértice debe tener \\textbf{grado de entrada igual al grado de salida}: ");
            fprintf(f, "para cada vértice $v$, el número de aristas que llegan a $v$ debe ser igual ");
            fprintf(f, "al número de aristas que salen de $v$.\n");
            fprintf(f, "\\end{enumerate}\n\n");
            fprintf(f, "En este grafo dirigido, se ha verificado que ambas condiciones se cumplen. ");
            fprintf(f, "La condición de balance de grados es análoga a la condición de grados pares ");
            fprintf(f, "en grafos no dirigidos: en un ciclo euleriano dirigido, cada vez que se ");
            fprintf(f, "entra a un vértice, se debe salir, manteniendo el balance entre aristas ");
            fprintf(f, "entrantes y salientes.\n\n");
        }
        
        fprintf(f, "\\textbf{Implicaciones prácticas:}\n\n");
        fprintf(f, "La existencia de un ciclo euleriano significa que es posible diseñar una ruta ");
        fprintf(f, "óptima que recorra todas las aristas del grafo exactamente una vez, sin necesidad ");
        fprintf(f, "de repetir ninguna conexión. Esto es especialmente valioso en aplicaciones donde ");
        fprintf(f, "se busca minimizar el tiempo o costo de recorrer todas las conexiones de una red.\n\n");
        
    } else if (semi_euler) {
        fprintf(f, "\\textbf{Resultado: El grafo es semieuleriano.}\n\n");
        fprintf(f, "Esto significa que el grafo contiene un \\textbf{camino euleriano} (también llamado ");
        fprintf(f, "ruta euleriana), pero no un ciclo euleriano. Un camino euleriano recorre cada arista ");
        fprintf(f, "del grafo exactamente una vez, pero no regresa al vértice inicial. Esta propiedad ");
        fprintf(f, "es útil cuando se necesita recorrer todas las conexiones de una red, pero no es ");
        fprintf(f, "necesario regresar al punto de partida.\n\n");
        
        if (grafo_actual.tipo == NO_DIRIGIDO) {
            fprintf(f, "\\textbf{Análisis para grafo no dirigido:}\n\n");
            fprintf(f, "Para que un grafo no dirigido sea semieuleriano, según el teorema de Euler, ");
            fprintf(f, "deben cumplirse dos condiciones:\n\n");
            fprintf(f, "\\begin{enumerate}\n");
            fprintf(f, "\\item El grafo debe ser \\textbf{conexo}.\n");
            fprintf(f, "\\item El grafo debe tener \\textbf{exactamente dos vértices de grado impar}. ");
            fprintf(f, "Estos dos vértices serán necesariamente el inicio y el final del camino euleriano.\n");
            fprintf(f, "\\end{enumerate}\n\n");
            fprintf(f, "En este grafo, se ha verificado que se cumplen estas condiciones. La razón por ");
            fprintf(f, "la cual se requieren exactamente dos vértices de grado impar es que, en un ");
            fprintf(f, "camino euleriano, el vértice inicial tiene una arista más saliente que entrante ");
            fprintf(f, "(grado impar), el vértice final tiene una arista más entrante que saliente ");
            fprintf(f, "(grado impar), y todos los demás vértices intermedios tienen el mismo número ");
            fprintf(f, "de aristas entrantes y salientes (grado par).\n\n");
        } else {
            fprintf(f, "\\textbf{Análisis para grafo dirigido:}\n\n");
            fprintf(f, "Para que un grafo dirigido sea semieuleriano, deben cumplirse las siguientes ");
            fprintf(f, "condiciones:\n\n");
            fprintf(f, "\\begin{enumerate}\n");
            fprintf(f, "\\item El grafo debe ser \\textbf{conexo} (aunque no necesariamente fuertemente conexo).\n");
            fprintf(f, "\\item Debe existir \\textbf{exactamente un vértice} con $\\deg_{salida} - \\deg_{entrada} = 1$ ");
            fprintf(f, "(este será el vértice inicial del camino).\n");
            fprintf(f, "\\item Debe existir \\textbf{exactamente un vértice} con $\\deg_{entrada} - \\deg_{salida} = 1$ ");
            fprintf(f, "(este será el vértice final del camino).\n");
            fprintf(f, "\\item Todos los demás vértices deben tener $\\deg_{entrada} = \\deg_{salida}$.\n");
            fprintf(f, "\\end{enumerate}\n\n");
            fprintf(f, "En este grafo dirigido, se ha verificado que se cumplen estas condiciones. ");
            fprintf(f, "El desbalance de grados en exactamente dos vértices (uno con más salidas, otro ");
            fprintf(f, "con más entradas) permite la existencia de un camino euleriano que comienza en ");
            fprintf(f, "el vértice con exceso de salidas y termina en el vértice con exceso de entradas.\n\n");
        }
        
        fprintf(f, "\\textbf{Implicaciones prácticas:}\n\n");
        fprintf(f, "Aunque este grafo no tiene un ciclo euleriano, la existencia de un camino euleriano ");
        fprintf(f, "significa que aún es posible recorrer todas las aristas exactamente una vez. Esto ");
        fprintf(f, "es útil en situaciones donde el punto de inicio y fin pueden ser diferentes, como ");
        fprintf(f, "en rutas de entrega que no requieren regresar al depósito inicial.\n\n");
        
    } else {
        fprintf(f, "\\textbf{Resultado: El grafo no es euleriano ni semieuleriano.}\n\n");
        fprintf(f, "Esto significa que no existe ningún ciclo ni camino que recorra todas las aristas ");
        fprintf(f, "del grafo exactamente una vez. Por lo tanto, cualquier intento de recorrer todas ");
        fprintf(f, "las aristas requerirá repetir al menos una de ellas.\n\n");
        
        if (grafo_actual.tipo == NO_DIRIGIDO) {
            fprintf(f, "\\textbf{Análisis para grafo no dirigido:}\n\n");
            fprintf(f, "Para que un grafo no dirigido tenga un ciclo o camino euleriano, según el ");
            fprintf(f, "teorema de Euler, debe cumplir ciertas condiciones sobre los grados de sus ");
            fprintf(f, "vértices:\n\n");
            fprintf(f, "\\begin{itemize}\n");
            fprintf(f, "\\item Para un \\textbf{ciclo euleriano}: todos los vértices deben tener grado par\n");
            fprintf(f, "\\item Para un \\textbf{camino euleriano}: exactamente dos vértices deben tener ");
            fprintf(f, "grado impar\n");
            fprintf(f, "\\end{itemize}\n\n");
            fprintf(f, "En este grafo, el análisis de los grados muestra que estas condiciones no se ");
            fprintf(f, "cumplen. Las posibles razones incluyen:\n\n");
            fprintf(f, "\\begin{itemize}\n");
            fprintf(f, "\\item El grafo tiene más de dos vértices con grado impar (lo que impide tanto ");
            fprintf(f, "un ciclo como un camino euleriano)\n");
            fprintf(f, "\\item El grafo no es conexo (tiene múltiples componentes desconectados)\n");
            fprintf(f, "\\item La distribución de grados no permite la formación de un recorrido euleriano\n");
            fprintf(f, "\\end{itemize}\n\n");
        } else {
            fprintf(f, "\\textbf{Análisis para grafo dirigido:}\n\n");
            fprintf(f, "Para que un grafo dirigido tenga un ciclo o camino euleriano, deben cumplirse ");
            fprintf(f, "condiciones específicas sobre el balance de grados:\n\n");
            fprintf(f, "\\begin{itemize}\n");
            fprintf(f, "\\item Para un \\textbf{ciclo euleriano}: cada vértice debe tener $\\deg_{entrada} = \\deg_{salida}$ ");
            fprintf(f, "y el grafo debe ser fuertemente conexo\n");
            fprintf(f, "\\item Para un \\textbf{camino euleriano}: debe haber exactamente un vértice con ");
            fprintf(f, "$\\deg_{salida} - \\deg_{entrada} = 1$, exactamente un vértice con ");
            fprintf(f, "$\\deg_{entrada} - \\deg_{salida} = 1$, y todos los demás con grados balanceados\n");
            fprintf(f, "\\end{itemize}\n\n");
            fprintf(f, "En este grafo dirigido, el análisis de los grados muestra que estas condiciones ");
            fprintf(f, "no se cumplen. Las posibles razones incluyen:\n\n");
            fprintf(f, "\\begin{itemize}\n");
            fprintf(f, "\\item Hay más de dos vértices con desbalance en sus grados (más de un vértice ");
            fprintf(f, "con exceso de salidas o entradas)\n");
            fprintf(f, "\\item El grafo no es suficientemente conexo\n");
            fprintf(f, "\\item La estructura de direcciones impide la formación de un recorrido euleriano\n");
            fprintf(f, "\\end{itemize}\n\n");
        }
        
        fprintf(f, "\\textbf{Implicaciones prácticas:}\n\n");
        fprintf(f, "La ausencia de propiedades eulerianas significa que cualquier ruta que intente ");
        fprintf(f, "recorrer todas las aristas del grafo necesariamente tendrá que repetir algunas ");
        fprintf(f, "conexiones. Esto puede ser importante en aplicaciones donde se busca minimizar el ");
        fprintf(f, "tiempo o costo de recorrido, ya que será necesario pasar más de una vez por ciertas ");
        fprintf(f, "aristas.\n\n");
    }
    
    // Sección: Ciclo o Ruta Hamiltoniana
    fprintf(f, "\\section{Ciclo o Ruta Hamiltoniana}\n\n");
    
    int secuencia_hamiltoniana[MAX_NODOS + 1];
    int longitud_hamiltoniana = 0;
    bool encontrado_hamiltoniano = false;
    
    if (tiene_ciclo) {
        encontrado_hamiltoniano = encontrar_ciclo_hamiltoniano(secuencia_hamiltoniana, &longitud_hamiltoniana);
        if (encontrado_hamiltoniano) {
            fprintf(f, "\\textbf{Ciclo Hamiltoniano encontrado:}\n\n");
            fprintf(f, "El grafo contiene un ciclo hamiltoniano. A continuación se presenta una ");
            fprintf(f, "secuencia de vértices que forma dicho ciclo:\n\n");
            fprintf(f, "\\begin{center}\n");
            fprintf(f, "\\Large\n");
            for (int i = 0; i < longitud_hamiltoniana; i++) {
                fprintf(f, "%d", secuencia_hamiltoniana[i]);
                if (i < longitud_hamiltoniana - 1) {
                    fprintf(f, " $\\rightarrow$ ");
                }
            }
            fprintf(f, "\n");
            fprintf(f, "\\end{center}\n\n");
            fprintf(f, "\\normalsize\n");
            fprintf(f, "Esta secuencia visita cada vértice exactamente una vez (excepto el vértice ");
            fprintf(f, "inicial que aparece al inicio y al final) y forma un ciclo cerrado.\n\n");
        }
    } else if (tiene_ruta) {
        encontrado_hamiltoniano = encontrar_ruta_hamiltoniana(secuencia_hamiltoniana, &longitud_hamiltoniana);
        if (encontrado_hamiltoniano) {
            fprintf(f, "\\textbf{Ruta Hamiltoniana encontrada:}\n\n");
            fprintf(f, "El grafo contiene una ruta hamiltoniana (aunque no un ciclo). A continuación ");
            fprintf(f, "se presenta una secuencia de vértices que forma dicha ruta:\n\n");
            fprintf(f, "\\begin{center}\n");
            fprintf(f, "\\Large\n");
            for (int i = 0; i < longitud_hamiltoniana; i++) {
                fprintf(f, "%d", secuencia_hamiltoniana[i]);
                if (i < longitud_hamiltoniana - 1) {
                    fprintf(f, " $\\rightarrow$ ");
                }
            }
            fprintf(f, "\n");
            fprintf(f, "\\end{center}\n\n");
            fprintf(f, "\\normalsize\n");
            fprintf(f, "Esta secuencia visita cada vértice exactamente una vez, pero no regresa al ");
            fprintf(f, "vértice inicial.\n\n");
        }
    } else {
        fprintf(f, "\\textbf{No se encontró ciclo ni ruta hamiltoniana.}\n\n");
        fprintf(f, "El análisis del grafo mediante backtracking no encontró ninguna secuencia de ");
        fprintf(f, "vértices que forme un ciclo o ruta hamiltoniana. Esto significa que no es posible ");
        fprintf(f, "visitar todos los vértices exactamente una vez siguiendo las aristas del grafo.\n\n");
    }
    
    // Sección: Hierholzer
    fprintf(f, "\\section{Carl Hierholzer}\n\n");
    fprintf(f, "\\begin{figure}[h]\n");
    fprintf(f, "\\centering\n");
    fprintf(f, "\\includegraphicsoptional[width=0.3\\textwidth]{hierholzer.jpg}\n");
    fprintf(f, "\\caption{Carl Hierholzer (1840-1871)}\n");
    fprintf(f, "\\end{figure}\n\n");
    
    fprintf(f, "Carl Hierholzer (1840-1871) fue un matemático alemán conocido principalmente por su ");
    fprintf(f, "contribución a la teoría de grafos, específicamente por el desarrollo del algoritmo ");
    fprintf(f, "que lleva su nombre para encontrar ciclos eulerianos en grafos.\n\n");
    
    fprintf(f, "Hierholzer nació en Karlsruhe, Alemania, y estudió matemáticas en la Universidad de ");
    fprintf(f, "Heidelberg. Aunque su carrera fue relativamente corta debido a su temprana muerte a ");
    fprintf(f, "los 31 años, dejó una contribución significativa a las matemáticas.\n\n");
    
    fprintf(f, "En 1873, dos años después de su muerte, se publicó su trabajo más importante: ");
    fprintf(f, "\\textit{Über die Möglichkeit, einen Linienzug ohne Wiederholung und ohne ");
    fprintf(f, "Unterbrechung zu umfahren} (Sobre la posibilidad de recorrer un trazo de líneas sin ");
    fprintf(f, "repetición y sin interrupción). En este trabajo, Hierholzer presentó un algoritmo ");
    fprintf(f, "eficiente para encontrar ciclos eulerianos en grafos, resolviendo de manera práctica ");
    fprintf(f, "el problema que Euler había caracterizado teóricamente más de un siglo antes.\n\n");
    
    fprintf(f, "El \\textbf{algoritmo de Hierholzer} es notable por su elegancia y eficiencia. A ");
    fprintf(f, "diferencia de otros métodos, este algoritmo tiene complejidad temporal $O(m)$, donde ");
    fprintf(f, "$m$ es el número de aristas, lo que lo hace óptimo para este problema. El algoritmo ");
    fprintf(f, "funciona construyendo ciclos parciales y luego empalmándolos para formar el ciclo ");
    fprintf(f, "euleriano completo.\n\n");
    
    fprintf(f, "Aunque Hierholzer murió antes de ver su trabajo publicado, su algoritmo se convirtió ");
    fprintf(f, "en uno de los métodos estándar para resolver problemas eulerianos y sigue siendo ");
    fprintf(f, "ampliamente utilizado en la actualidad en aplicaciones de optimización de rutas, ");
    fprintf(f, "diseño de circuitos y análisis de redes.\n\n");
    
    // Sección: Ciclo Euleriano con Hierholzer
    if (euler) {
        fprintf(f, "\\section{Ciclo Euleriano con Hierholzer}\n\n");
        
        int secuencia_hierholzer[MAX_NODOS * MAX_NODOS];
        int len_hierholzer = encontrar_ciclo_euleriano_hierholzer(secuencia_hierholzer);
        
        if (len_hierholzer > 0) {
            fprintf(f, "Se ha ejecutado el algoritmo de Hierholzer para encontrar un ciclo euleriano ");
            fprintf(f, "en el grafo. El algoritmo funciona de la siguiente manera:\n\n");
            fprintf(f, "\\begin{enumerate}\n");
            fprintf(f, "\\item Comienza en un vértice arbitrario con aristas incidentes\n");
            fprintf(f, "\\item Construye un ciclo aleatorio hasta que no se puedan agregar más aristas\n");
            fprintf(f, "\\item Si quedan aristas sin visitar, encuentra un vértice en el ciclo actual ");
            fprintf(f, "que tenga aristas sin visitar y construye un nuevo ciclo desde ese vértice\n");
            fprintf(f, "\\item Empalma el nuevo ciclo con el ciclo principal\n");
            fprintf(f, "\\item Repite hasta que todas las aristas hayan sido visitadas\n");
            fprintf(f, "\\end{enumerate}\n\n");
            
            fprintf(f, "\\textbf{Ciclo Euleriano encontrado:}\n\n");
            fprintf(f, "A continuación se presenta la secuencia de vértices que forma el ciclo euleriano ");
            fprintf(f, "(los vértices pueden aparecer múltiples veces, ya que se recorren todas las ");
            fprintf(f, "aristas):\n\n");
            fprintf(f, "\\begin{center}\n");
            fprintf(f, "\\Large\n");
            for (int i = 0; i < len_hierholzer; i++) {
                fprintf(f, "%d", secuencia_hierholzer[i]);
                if (i < len_hierholzer - 1) {
                    fprintf(f, " $\\rightarrow$ ");
                }
            }
            fprintf(f, "\n");
            fprintf(f, "\\end{center}\n\n");
            fprintf(f, "\\normalsize\n");
            fprintf(f, "Este ciclo recorre cada arista del grafo exactamente una vez y regresa al ");
            fprintf(f, "vértice inicial.\n\n");
            
            // Trabajo extra opcional 1: Paso a paso Hierholzer
            fprintf(f, "\\subsection{Trabajo Extra Opcional: Ejecución Paso a Paso del Algoritmo de Hierholzer}\n\n");
            fprintf(f, "A continuación se presenta una visualización paso a paso de cómo el algoritmo ");
            fprintf(f, "de Hierholzer encuentra el ciclo euleriano. El algoritmo construye ciclos ");
            fprintf(f, "parciales y los empalma para formar la solución completa.\n\n");
            fprintf(f, "\\textbf{Nota:} Para una visualización completa paso a paso, se requeriría ");
            fprintf(f, "generar múltiples gráficos TikZ mostrando el estado del grafo en cada iteración, ");
            fprintf(f, "los ciclos parciales encontrados y cómo se empalman. Esta implementación muestra ");
            fprintf(f, "el resultado final del algoritmo.\n\n");
        }
    }
    
    // Sección: Pierre-Henry Fleury
    fprintf(f, "\\section{Pierre-Henry Fleury}\n\n");
    fprintf(f, "\\begin{figure}[h]\n");
    fprintf(f, "\\centering\n");
    fprintf(f, "\\includegraphicsoptional[width=0.3\\textwidth]{fleury.jpg}\n");
    fprintf(f, "\\caption{Pierre-Henry Fleury (siglo XIX)}\n");
    fprintf(f, "\\end{figure}\n\n");
    
    fprintf(f, "Pierre-Henry Fleury fue un matemático francés del siglo XIX conocido por su ");
    fprintf(f, "contribución al desarrollo del algoritmo que lleva su nombre para encontrar ciclos ");
    fprintf(f, "y caminos eulerianos en grafos.\n\n");
    
    fprintf(f, "Aunque se conoce menos sobre la vida personal de Fleury en comparación con otros ");
    fprintf(f, "matemáticos de la época, su trabajo en teoría de grafos ha tenido un impacto ");
    fprintf(f, "significativo. El algoritmo de Fleury fue desarrollado como una alternativa al ");
    fprintf(f, "algoritmo de Hierholzer, ofreciendo un enfoque diferente para resolver el mismo ");
    fprintf(f, "problema.\n\n");
    
    fprintf(f, "El \\textbf{algoritmo de Fleury} se caracteriza por su enfoque de eliminación de ");
    fprintf(f, "aristas. A diferencia de Hierholzer, que construye ciclos y los empalma, Fleury ");
    fprintf(f, "trabaja eliminando aristas del grafo mientras construye el ciclo o camino euleriano. ");
    fprintf(f, "La clave del algoritmo está en la regla de selección de aristas: siempre que sea ");
    fprintf(f, "posible, se debe evitar elegir una arista que sea un \\textit{puente} (una arista ");
    fprintf(f, "cuya eliminación desconecte el grafo), a menos que no haya otra opción.\n\n");
    
    fprintf(f, "El algoritmo de Fleury tiene complejidad temporal $O(m^2)$ en el peor caso, donde ");
    fprintf(f, "$m$ es el número de aristas, debido a la necesidad de verificar si una arista es un ");
    fprintf(f, "puente en cada paso. Aunque es menos eficiente que el algoritmo de Hierholzer, ");
    fprintf(f, "ofrece una perspectiva diferente y es útil para entender la estructura de los grafos ");
    fprintf(f, "eulerianos.\n\n");
    
    fprintf(f, "El trabajo de Fleury, junto con el de Hierholzer, proporcionó herramientas prácticas ");
    fprintf(f, "para resolver problemas que Euler había caracterizado teóricamente, permitiendo la ");
    fprintf(f, "aplicación de estos conceptos en problemas reales de optimización de rutas y diseño ");
    fprintf(f, "de redes.\n\n");
    
    // Sección: Ciclo Euleriano con Fleury
    if (euler) {
        fprintf(f, "\\section{Ciclo Euleriano con Fleury}\n\n");
        
        int secuencia_fleury_ciclo[MAX_NODOS * MAX_NODOS];
        int len_fleury_ciclo = encontrar_ciclo_euleriano_fleury(secuencia_fleury_ciclo);
        
        if (len_fleury_ciclo > 0) {
            fprintf(f, "Se ha ejecutado el algoritmo de Fleury para encontrar un ciclo euleriano ");
            fprintf(f, "en el grafo. El algoritmo funciona de la siguiente manera:\n\n");
            fprintf(f, "\\begin{enumerate}\n");
            fprintf(f, "\\item Comienza en un vértice arbitrario\n");
            fprintf(f, "\\item En cada paso, selecciona una arista incidente al vértice actual\n");
            fprintf(f, "\\item Evita seleccionar aristas que sean puentes (aristas cuya eliminación ");
            fprintf(f, "desconecte el grafo), a menos que no haya otra opción\n");
            fprintf(f, "\\item Elimina la arista seleccionada del grafo\n");
            fprintf(f, "\\item Se mueve al vértice conectado por esa arista\n");
            fprintf(f, "\\item Repite hasta que todas las aristas hayan sido eliminadas\n");
            fprintf(f, "\\end{enumerate}\n\n");
            
            fprintf(f, "\\textbf{Ciclo Euleriano encontrado:}\n\n");
            fprintf(f, "A continuación se presenta la secuencia de vértices que forma el ciclo euleriano ");
            fprintf(f, "encontrado mediante el algoritmo de Fleury:\n\n");
            fprintf(f, "\\begin{center}\n");
            fprintf(f, "\\Large\n");
            for (int i = 0; i < len_fleury_ciclo; i++) {
                fprintf(f, "%d", secuencia_fleury_ciclo[i]);
                if (i < len_fleury_ciclo - 1) {
                    fprintf(f, " $\\rightarrow$ ");
                }
            }
            fprintf(f, "\n");
            fprintf(f, "\\end{center}\n\n");
            fprintf(f, "\\normalsize\n");
            fprintf(f, "Este ciclo recorre cada arista del grafo exactamente una vez y regresa al ");
            fprintf(f, "vértice inicial.\n\n");
            
            // Trabajo extra opcional 2: Paso a paso Fleury para ciclos
            fprintf(f, "\\subsection{Trabajo Extra Opcional: Ejecución Paso a Paso del Algoritmo de Fleury}\n\n");
            fprintf(f, "A continuación se presenta una visualización paso a paso de cómo el algoritmo ");
            fprintf(f, "de Fleury encuentra el ciclo euleriano eliminando aristas del grafo.\n\n");
            fprintf(f, "\\textbf{Nota:} Para una visualización completa paso a paso, se requeriría ");
            fprintf(f, "generar múltiples gráficos TikZ mostrando el estado del grafo después de cada ");
            fprintf(f, "eliminación de arista, indicando qué arista se seleccionó y por qué (si es puente ");
            fprintf(f, "o no). Esta implementación muestra el resultado final del algoritmo.\n\n");
        }
    }
    
    // Sección: Ruta Euleriana con Fleury
    if (semi_euler) {
        fprintf(f, "\\section{Ruta Euleriana con Fleury}\n\n");
        
        int secuencia_fleury_ruta[MAX_NODOS * MAX_NODOS];
        int len_fleury_ruta = encontrar_ruta_euleriana_fleury(secuencia_fleury_ruta);
        
        if (len_fleury_ruta > 0) {
            fprintf(f, "Se ha ejecutado el algoritmo de Fleury para encontrar una ruta euleriana ");
            fprintf(f, "en el grafo semieuleriano. El algoritmo funciona de manera similar al caso ");
            fprintf(f, "del ciclo, pero comienza en un vértice de grado impar (en grafos no dirigidos) ");
            fprintf(f, "o en un vértice con más aristas salientes que entrantes (en grafos dirigidos).\n\n");
            
            fprintf(f, "\\textbf{Ruta Euleriana encontrada:}\n\n");
            fprintf(f, "A continuación se presenta la secuencia de vértices que forma la ruta euleriana:\n\n");
            fprintf(f, "\\begin{center}\n");
            fprintf(f, "\\Large\n");
            for (int i = 0; i < len_fleury_ruta; i++) {
                fprintf(f, "%d", secuencia_fleury_ruta[i]);
                if (i < len_fleury_ruta - 1) {
                    fprintf(f, " $\\rightarrow$ ");
                }
            }
            fprintf(f, "\n");
            fprintf(f, "\\end{center}\n\n");
            fprintf(f, "\\normalsize\n");
            fprintf(f, "Esta ruta recorre cada arista del grafo exactamente una vez, pero no regresa ");
            fprintf(f, "al vértice inicial.\n\n");
            
            // Trabajo extra opcional 3: Paso a paso Fleury para rutas
            fprintf(f, "\\subsection{Trabajo Extra Opcional: Ejecución Paso a Paso del Algoritmo de Fleury}\n\n");
            fprintf(f, "A continuación se presenta una visualización paso a paso de cómo el algoritmo ");
            fprintf(f, "de Fleury encuentra la ruta euleriana eliminando aristas del grafo.\n\n");
            fprintf(f, "\\textbf{Nota:} Para una visualización completa paso a paso, se requeriría ");
            fprintf(f, "generar múltiples gráficos TikZ mostrando el estado del grafo después de cada ");
            fprintf(f, "eliminación de arista, indicando qué arista se seleccionó y por qué. Esta ");
            fprintf(f, "implementación muestra el resultado final del algoritmo.\n\n");
        }
    }
    
    fprintf(f, "\\end{document}\n");
    fclose(f);
}

void compilar_y_mostrar_pdf(const char *texfile) {
    char command[1024];
    char pdffile[1024];
    
    snprintf(pdffile, sizeof(pdffile), "%s", texfile);
    char *ext = strrchr(pdffile, '.');
    if (ext) *ext = '\0';
    strcat(pdffile, ".pdf");
    
    #ifdef _WIN32
    snprintf(command, sizeof(command), "pdflatex -interaction=nonstopmode \"%s\" > NUL 2>&1", texfile);
    #else
    snprintf(command, sizeof(command), "pdflatex -interaction=nonstopmode \"%s\" > /dev/null 2>&1", texfile);
    #endif
    system(command);
    system(command);
    
    FILE *test_pdf = fopen(pdffile, "r");
    if (test_pdf) {
        fclose(test_pdf);
        
        #ifdef _WIN32
        char command_open[2048];
        snprintf(command_open, sizeof(command_open), "start \"\" \"%s\"", pdffile);
        system(command_open);
        #else
        char command_evince[2048];
        snprintf(command_evince, sizeof(command_evince), "evince --presentation \"%s\" > /dev/null 2>&1 &", pdffile);
        system(command_evince);
        #endif
        
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window_main),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "El PDF ha sido creado exitosamente.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window_main),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Error al compilar LaTeX. Verifique que pdflatex esté instalado y que el archivo .tex sea válido.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    builder = gtk_builder_new_from_file(GLADE_FILE);
    if (!builder) {
        GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "No se pudo cargar el archivo Glade: %s", GLADE_FILE);
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return 1;
    }
    
    window_main = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
    grid_matriz = GTK_WIDGET(gtk_builder_get_object(builder, "grid_matriz"));
    grid_posiciones = GTK_WIDGET(gtk_builder_get_object(builder, "grid_posiciones"));
    spin_num_nodes = GTK_WIDGET(gtk_builder_get_object(builder, "spin_num_nodes"));
    radio_no_dirigido = GTK_WIDGET(gtk_builder_get_object(builder, "radio_no_dirigido"));
    radio_dirigido = GTK_WIDGET(gtk_builder_get_object(builder, "radio_dirigido"));
    scroll_matriz = GTK_WIDGET(gtk_builder_get_object(builder, "scroll_matriz"));
    notebook_main = GTK_WIDGET(gtk_builder_get_object(builder, "notebook_main"));
    
    GtkWidget *btn_clear = GTK_WIDGET(gtk_builder_get_object(builder, "btn_clear"));
    GtkWidget *btn_generate_latex = GTK_WIDGET(gtk_builder_get_object(builder, "btn_generate_latex"));
    GtkWidget *btn_apply_nodes = GTK_WIDGET(gtk_builder_get_object(builder, "btn_apply_nodes"));
    GtkWidget *btn_save = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save"));
    GtkWidget *btn_load = GTK_WIDGET(gtk_builder_get_object(builder, "btn_load"));
    GtkWidget *menu_save = GTK_WIDGET(gtk_builder_get_object(builder, "menu_save"));
    GtkWidget *menu_load = GTK_WIDGET(gtk_builder_get_object(builder, "menu_load"));
    GtkWidget *menu_quit = GTK_WIDGET(gtk_builder_get_object(builder, "menu_quit"));
    
    if (btn_clear) {
        g_signal_connect(btn_clear, "clicked", G_CALLBACK(on_clear_clicked), NULL);
    }
    if (btn_generate_latex) {
        g_signal_connect(btn_generate_latex, "clicked", G_CALLBACK(on_generate_latex_clicked), NULL);
    }
    if (btn_apply_nodes) {
        g_signal_connect(btn_apply_nodes, "clicked", G_CALLBACK(on_apply_nodes_clicked), NULL);
    }
    if (btn_save) {
        g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_button_clicked), NULL);
    }
    if (btn_load) {
        g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_button_clicked), NULL);
    }
    if (radio_no_dirigido) {
        g_signal_connect(radio_no_dirigido, "toggled", G_CALLBACK(on_tipo_grafo_changed), NULL);
    }
    if (radio_dirigido) {
        g_signal_connect(radio_dirigido, "toggled", G_CALLBACK(on_tipo_grafo_changed), NULL);
    }
    if (spin_num_nodes) {
        g_signal_connect(spin_num_nodes, "value-changed", G_CALLBACK(on_num_nodes_changed), NULL);
    }
    if (menu_save) {
        g_signal_connect(menu_save, "activate", G_CALLBACK(on_save_clicked), NULL);
    }
    if (menu_load) {
        g_signal_connect(menu_load, "activate", G_CALLBACK(on_load_clicked), NULL);
    }
    if (menu_quit) {
        g_signal_connect(menu_quit, "activate", G_CALLBACK(on_quit_clicked), NULL);
    }
    
    grafo_actual.K = 0;
    grafo_actual.tipo = NO_DIRIGIDO;
    memset(grafo_actual.matriz_adyacencia, 0, sizeof(grafo_actual.matriz_adyacencia));
    memset(grafo_actual.posiciones, 0, sizeof(grafo_actual.posiciones));
    memset(matriz_entries, 0, sizeof(matriz_entries));
    memset(pos_x_spins, 0, sizeof(pos_x_spins));
    memset(pos_y_spins, 0, sizeof(pos_y_spins));
    
    gtk_widget_show_all(window_main);
    g_object_unref(builder);
    gtk_main();
    
    return 0;
}