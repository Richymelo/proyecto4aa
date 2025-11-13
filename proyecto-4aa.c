
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
bool es_euleriano();
bool es_semieuleriano();
void generar_latex(const char *filename);
void compilar_y_mostrar_pdf(const char *texfile);

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
        char str[2];
        snprintf(str, 2, "%d", valor);
        gtk_entry_set_text(matriz_entries[col][fila], str);
    }
}

void on_save_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    (void)user_data;
    if (num_nodos_actual == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window_main),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Primero debe configurar el número de nodos");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
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
        }
        
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

void on_load_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    (void)user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Cargar Grafo",
        GTK_WINDOW(window_main), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancelar", GTK_RESPONSE_CANCEL,
        "_Abrir", GTK_RESPONSE_ACCEPT, NULL);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        FILE *f = fopen(filename, "r");
        
        if (f) {
            int K, tipo;
            fscanf(f, "%d", &K);
            fscanf(f, "%d", &tipo);
            
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
                        fscanf(f, "%d", &grafo_actual.matriz_adyacencia[i][j]);
                    }
                }
                
                for (int i = 0; i < K; i++) {
                    fscanf(f, "%d %d", &grafo_actual.posiciones[i].x, &grafo_actual.posiciones[i].y);
                }
                
                limpiar_matriz();
                limpiar_posiciones();
                crear_matriz_ui(K);
                crear_posiciones_ui(K);
                
                for (int i = 0; i < K; i++) {
                    for (int j = 0; j < K; j++) {
                        if (matriz_entries[i][j]) {
                            char str[2];
                            snprintf(str, 2, "%d", grafo_actual.matriz_adyacencia[i][j]);
                            gtk_entry_set_text(matriz_entries[i][j], str);
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
            }
            
            fclose(f);
        }
        
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
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
            gtk_entry_set_text(GTK_ENTRY(entry), "0");
            gtk_entry_set_width_chars(GTK_ENTRY(entry), 2);
            
            if (i == j) {
                gtk_widget_set_sensitive(entry, FALSE);
                gtk_entry_set_text(GTK_ENTRY(entry), "0");
            }
            
            int *coords = malloc(2 * sizeof(int));
            coords[0] = i;
            coords[1] = j;
            
            g_object_set_data(G_OBJECT(entry), "coords", coords);
            g_signal_connect(entry, "changed", G_CALLBACK(on_matriz_changed), coords);
            
            gtk_grid_attach(GTK_GRID(grid_matriz), entry, j + 1, i + 1, 1, 1);
            matriz_entries[i][j] = GTK_ENTRY(entry);
            grafo_actual.matriz_adyacencia[i][j] = 0;
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
        
        GtkAdjustment *adj_x = gtk_adjustment_new(0, 0, 1000, 1, 10, 0);
        GtkWidget *spin_x = gtk_spin_button_new(adj_x, 1, 0);
        gtk_grid_attach(GTK_GRID(grid_posiciones), spin_x, 1, i + 1, 1, 1);
        pos_x_spins[i] = GTK_SPIN_BUTTON(spin_x);
        
        GtkAdjustment *adj_y = gtk_adjustment_new(0, 0, 1000, 1, 10, 0);
        GtkWidget *spin_y = gtk_spin_button_new(adj_y, 1, 0);
        gtk_grid_attach(GTK_GRID(grid_posiciones), spin_y, 2, i + 1, 1, 1);
        pos_y_spins[i] = GTK_SPIN_BUTTON(spin_y);
        
        grafo_actual.posiciones[i].x = 0;
        grafo_actual.posiciones[i].y = 0;
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
    fprintf(f, "\\geometry{a4paper, margin=2.5cm}\n");
    fprintf(f, "\\title{Proyecto 4: Hamilton, Euler y Grafos, Parte I}\n");
    fprintf(f, "\\author{Miembros del Grupo:\\\\Ricardo Castro\\\\Juan Carlos Valverde\\\\~\\\\Curso: Analisis de Algoritmos\\\\~\\\\Semestres: II 2025}\n");
    fprintf(f, "\\date{\\today}\n\n");
    fprintf(f, "\\begin{document}\n\n");
    
    fprintf(f, "\\maketitle\n\n");
    fprintf(f, "\\thispagestyle{empty}\n\n");
    fprintf(f, "\\newpage\n\n");
    
    fprintf(f, "\\section{William Rowan Hamilton}\n\n");
    fprintf(f, "William Rowan Hamilton (1805-1865) fue un matemático, físico y astrónomo irlandés. ");
    fprintf(f, "Hizo importantes contribuciones al desarrollo del álgebra, la óptica y la mecánica. ");
    fprintf(f, "Es especialmente conocido por su trabajo en el álgebra de cuaterniones y por el ");
    fprintf(f, "problema del ciclo hamiltoniano, que lleva su nombre. El problema consiste en ");
    fprintf(f, "encontrar un ciclo en un grafo que visite cada vértice exactamente una vez.\n\n");
    
    fprintf(f, "\\section{Ciclos y Rutas Hamiltonianas}\n\n");
    fprintf(f, "Un \\textbf{ciclo hamiltoniano} es un ciclo en un grafo que visita cada vértice ");
    fprintf(f, "exactamente una vez y regresa al vértice inicial. Una \\textbf{ruta hamiltoniana} ");
    fprintf(f, "es un camino simple que visita cada vértice exactamente una vez, pero no necesariamente ");
    fprintf(f, "regresa al punto de partida.\n\n");
    fprintf(f, "El problema de determinar si un grafo tiene un ciclo o ruta hamiltoniana es un problema ");
    fprintf(f, "NP-completo. En este proyecto, se utiliza un algoritmo de backtracking para determinar ");
    fprintf(f, "si existe al menos un ciclo o ruta hamiltoniana, aunque no se encuentra la solución ");
    fprintf(f, "específica.\n\n");
    
    fprintf(f, "\\section{Leonhard Euler}\n\n");
    fprintf(f, "Leonhard Euler (1707-1783) fue un matemático y físico suizo considerado uno de los ");
    fprintf(f, "matemáticos más prolíficos de la historia. Realizó importantes descubrimientos en ");
    fprintf(f, "cálculo, teoría de grafos, teoría de números y muchas otras áreas. El problema de los ");
    fprintf(f, "puentes de Königsberg, que resolvió, es considerado el origen de la teoría de grafos.\n\n");
    
    fprintf(f, "\\section{Ciclos y Rutas Eulerianas}\n\n");
    fprintf(f, "Un \\textbf{ciclo euleriano} es un ciclo que recorre cada arista del grafo exactamente ");
    fprintf(f, "una vez y regresa al vértice inicial. Un \\textbf{camino euleriano} (o ruta euleriana) ");
    fprintf(f, "es un camino que recorre cada arista exactamente una vez, pero no necesariamente regresa ");
    fprintf(f, "al punto de partida.\n\n");
    fprintf(f, "Un grafo es \\textbf{euleriano} si tiene un ciclo euleriano. Un grafo es ");
    fprintf(f, "\\textbf{semieuleriano} si tiene un camino euleriano pero no un ciclo euleriano.\n\n");
    fprintf(f, "Para grafos no dirigidos: un grafo es euleriano si y solo si es conexo y todos los ");
    fprintf(f, "vértices tienen grado par. Es semieuleriano si es conexo y tiene exactamente dos ");
    fprintf(f, "vértices de grado impar.\n\n");
    fprintf(f, "Para grafos dirigidos: un grafo es euleriano si y solo si es fuertemente conexo y ");
    fprintf(f, "cada vértice tiene el mismo grado de entrada que de salida. Es semieuleriano si es ");
    fprintf(f, "conexo y tiene exactamente un vértice con grado de salida mayor que el de entrada ");
    fprintf(f, "en una unidad, y exactamente un vértice con grado de entrada mayor que el de salida ");
    fprintf(f, "en una unidad, y todos los demás tienen grados iguales.\n\n");
    
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
    if (tiene_ciclo) {
        fprintf(f, "El grafo \\textbf{contiene al menos un ciclo hamiltoniano}. Esto significa que ");
        fprintf(f, "existe un ciclo que visita cada vértice exactamente una vez y regresa al vértice ");
        fprintf(f, "inicial.\n\n");
    } else {
        fprintf(f, "El grafo \\textbf{no contiene un ciclo hamiltoniano}. No existe un ciclo que ");
        fprintf(f, "visite cada vértice exactamente una vez.\n\n");
    }
    
    if (tiene_ruta) {
        fprintf(f, "El grafo \\textbf{contiene al menos una ruta hamiltoniana}. Esto significa que ");
        fprintf(f, "existe un camino que visita cada vértice exactamente una vez.\n\n");
    } else {
        fprintf(f, "El grafo \\textbf{no contiene una ruta hamiltoniana}. No existe un camino que ");
        fprintf(f, "visite cada vértice exactamente una vez.\n\n");
    }
    
    fprintf(f, "\\subsection{Propiedades Eulerianas}\n\n");
    if (euler) {
        fprintf(f, "El grafo es \\textbf{euleriano}. Esto significa que tiene un ciclo euleriano, ");
        fprintf(f, "es decir, un ciclo que recorre cada arista exactamente una vez.\n\n");
        if (grafo_actual.tipo == NO_DIRIGIDO) {
            fprintf(f, "En un grafo no dirigido, esto requiere que sea conexo y que todos los ");
            fprintf(f, "vértices tengan grado par.\n\n");
        } else {
            fprintf(f, "En un grafo dirigido, esto requiere que sea fuertemente conexo y que cada ");
            fprintf(f, "vértice tenga el mismo grado de entrada que de salida.\n\n");
        }
    } else if (semi_euler) {
        fprintf(f, "El grafo es \\textbf{semieuleriano}. Esto significa que tiene un camino euleriano ");
        fprintf(f, "pero no un ciclo euleriano. Un camino euleriano recorre cada arista exactamente ");
        fprintf(f, "una vez.\n\n");
        if (grafo_actual.tipo == NO_DIRIGIDO) {
            fprintf(f, "En un grafo no dirigido, esto requiere que sea conexo y que tenga exactamente ");
            fprintf(f, "dos vértices de grado impar.\n\n");
        } else {
            fprintf(f, "En un grafo dirigido, esto requiere que tenga exactamente un vértice con ");
            fprintf(f, "grado de salida mayor que el de entrada en una unidad, y exactamente un ");
            fprintf(f, "vértice con grado de entrada mayor que el de salida en una unidad.\n\n");
        }
    } else {
        fprintf(f, "El grafo \\textbf{no es euleriano ni semieuleriano}. No existe un ciclo ni un ");
        fprintf(f, "camino que recorra cada arista exactamente una vez.\n\n");
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