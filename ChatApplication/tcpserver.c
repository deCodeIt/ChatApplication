#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define INVALID -1
#define QUEUELIMIT 10
#define BUFFER_SIZE 1024
#define QUIT_STRING "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

int client_socket=-1;

typedef struct {
  GtkWidget *gtkwidget;
} myGtkWidget;

typedef struct {
  GtkWidget *ip;
  GtkWidget *port;
  GtkWidget *window;
  GtkWidget *destroyIt;
  // myGtkWidget *Extra;

} server_data;

typedef struct{
  GtkTextIter iter; 
  GtkTextBuffer *buffer_res;
  char *buffer;
  int len;
  GtkWidget *message_entry;
  GtkWidget *textview;
  GtkWidget *scrollview;
  GtkWidget *btn_send;

} rec_buf_cont; //received buffer content so as to display the recived text with threading accordingly

void *myThreadFun(void *vargp)
{
    // sleep(1);
    int msg_len=1;
    int BUF_SIZE = BUFFER_SIZE;
    rec_buf_cont *rec1 = (rec_buf_cont *)vargp;
    char *buffer = malloc(sizeof(char)*(BUF_SIZE + 12));
    while((msg_len = recv(client_socket,rec1->buffer,BUF_SIZE,0))>0)
    {
      if(strncmp(rec1->buffer,QUIT_STRING,1024)==0)
      {
        //the client is down
        gtk_widget_set_sensitive(rec1->btn_send, FALSE);
        gtk_text_buffer_insert(rec1->buffer_res, &(rec1->iter), "CLIENT HAS LEFT THE CHATROOM\n" , -1);

        GtkTextMark *insert_mark = gtk_text_buffer_get_insert (rec1->buffer_res);

        /* move mark and selection bound to the end */
        gtk_text_buffer_place_cursor(rec1->buffer_res, &(rec1->iter));

        gtk_text_view_scroll_to_mark( GTK_TEXT_VIEW (rec1->textview),
            insert_mark, 0.0, TRUE, 0.0, 1.0);
        close(client_socket);
        // gtk_main_quit();
        return NULL;
      }
        buffer[0]='\0';
        strcat(buffer,"CLIENT: ");
        strcat(buffer,rec1->buffer);
        buffer[strlen(buffer)+1]='\0';
        buffer[strlen(buffer)]='\n';
        
        gtk_text_buffer_insert(rec1->buffer_res, &(rec1->iter), buffer , -1);
        GtkTextMark *insert_mark = gtk_text_buffer_get_insert (rec1->buffer_res);

        /* move mark and selection bound to the end */
        gtk_text_buffer_place_cursor(rec1->buffer_res, &(rec1->iter));

        gtk_text_view_scroll_to_mark( GTK_TEXT_VIEW (rec1->textview),
            insert_mark, 0.0, TRUE, 0.0, 1.0);
        
    }

    return NULL;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
// Creates the icon for my App :)

GdkPixbuf *create_pixbuf(const gchar * filename) {
    
   GdkPixbuf *pixbuf;
   GError *error = NULL;
   pixbuf = gdk_pixbuf_new_from_file(filename, &error);
   
   if (!pixbuf) {
       
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
   }

   return pixbuf;
}
////////////////////////////////////////////////
////////////////////////////////////////////////

void myQuitFunction()
{
  if(client_socket!=-1)
  {
    send(client_socket,QUIT_STRING,BUFFER_SIZE,0);
    close(client_socket);
  }
  gtk_main_quit();
}

void send_button_clicked(GtkWidget *widget, gpointer data) {
  gtk_widget_set_sensitive(widget, FALSE);
  int msg_len;
  short BUF_SIZE=BUFFER_SIZE;
  rec_buf_cont *rec1 = (rec_buf_cont *) data;
  if(strlen(gtk_entry_get_text(GTK_ENTRY(rec1->message_entry)))==0)
  {
    gtk_widget_set_sensitive(widget, TRUE);
    return;
  }
  if((msg_len=send(client_socket,gtk_entry_get_text(GTK_ENTRY(rec1->message_entry)),BUF_SIZE,0))==-1)
  {
    fprintf(stderr,"error while sending message to Client\n");
    return;
  }
  else
  {
    char *buffer = malloc(sizeof(char)*(BUF_SIZE));
    buffer[0]='\0';
    strcat(buffer,"YOU: ");
    strcat(buffer,gtk_entry_get_text(GTK_ENTRY(rec1->message_entry)));
    buffer[strlen(buffer)+1]='\0';
    buffer[strlen(buffer)]='\n';
    gtk_text_buffer_insert(rec1->buffer_res, &(rec1->iter), buffer , -1); 
    GtkTextMark *insert_mark = gtk_text_buffer_get_insert (rec1->buffer_res);

    /* move mark and selection bound to the end */
    gtk_text_buffer_place_cursor(rec1->buffer_res, &(rec1->iter));

    gtk_text_view_scroll_to_mark( GTK_TEXT_VIEW (rec1->textview),
        insert_mark, 0.0, TRUE, 0.0, 1.0);
    gtk_entry_set_text(GTK_ENTRY(rec1->message_entry),"");
    gtk_widget_set_sensitive(widget, TRUE);
    return;
  }
}

int main(int argc, char* argv[])
{

    gtk_init(&argc, &argv);

    int server_socket;
    struct sockaddr_in server_address_in,client_address_in;
    struct sockaddr server_address,client_address;
    unsigned short server_port;
    unsigned int client_address_length;
    pid_t pid;

    if(argc!=2)
    {
        // error: incorrect usage
        printf("Syntax: %s <server_port>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    server_port = atoi(argv[1]); // got the server port

    //try to create a socket to server
    if( (server_socket = socket(PF_INET,SOCK_STREAM,0))==-1)
    {
        printf("ERROR: Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // now the socket has been created, lets allocate the port and other stuffs

    memset(&server_address_in,0,sizeof(server_address_in));

    server_address_in.sin_family = AF_INET;
    server_address_in.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address_in.sin_port = htons(server_port);
    char *ip = inet_ntoa(server_address_in.sin_addr);
    // printf("Server IP: %s\n", ip );

    //binding to local
    server_address = *(struct sockaddr*) &server_address_in;
    if(bind(server_socket,&server_address,sizeof(server_address))==-1)
    {
        printf("ERROR: Binding failed\n");
        exit(EXIT_FAILURE);
    }

    //start listening
    
    printf("I (Server) am Listening on port: %d\n",server_port);
    
    if(listen(server_socket,QUEUELIMIT)==-1)
    {
        printf("ERROR: Listening Failed\n");
        exit(EXIT_FAILURE);
    }

    //run forever unless user breaks or a client gets connected
    client_address_length = sizeof(client_address_in);

    //let the client connect to server
    if((client_socket = accept(server_socket,&client_address,&client_address_length)) == -1)
    {
        printf("ERROR: Accept Failed\n");
        exit(EXIT_FAILURE);
    }

    // now do whatever with your client
    
    // to display server chats
    GtkWidget *window;
    GtkWidget *vbox,*hbox;
    GdkPixbuf *icon;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Server");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 15);

    
    GtkWidget *scrollview = gtk_scrolled_window_new( NULL, NULL ); 
    GtkWidget *textview = gtk_text_view_new();
    GtkWidget *message_entry = gtk_entry_new();

    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview),FALSE);

    GtkWidget *btn_send = gtk_button_new_with_label("Send");
    gtk_widget_set_tooltip_text(btn_send, "Send this message to the Client");
    gtk_widget_set_size_request(btn_send,70,30);

    // now create a new vbox and hbox view to send and recieve message from the server
    GtkTextIter iter; // used by textview buffer
    GtkTextBuffer *buffer_res = gtk_text_view_get_buffer(GTK_TEXT_VIEW (textview)); 
    gtk_text_buffer_get_start_iter(buffer_res, &iter); // for server messages display with buffer

    // table = gtk_table_new(10, 5, TRUE);

    vbox = gtk_vbox_new(FALSE, 1); // a ertical layout that will divide into 2, top => received messaged, bottom => hbox
    hbox = gtk_hbox_new(FALSE, 1); // divides horizontal layout by 2, left => send text entry , right => send button

    gtk_container_add(GTK_CONTAINER (scrollview), textview); // append the text view to scroll view
    gtk_box_pack_start(GTK_BOX(vbox), scrollview, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), message_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), btn_send, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER (window), vbox);


    gtk_widget_show_all(window);

    // gtk_text_buffer_insert(buffer_res, &iter, "This is a test line\n" , -1); 

    icon = create_pixbuf("chat_img.png");
    gtk_window_set_icon(GTK_WINDOW(window), icon);

    gtk_widget_show_all(window);

    char *buffer;
    short BUF_SIZE = BUFFER_SIZE;
    buffer = (char *)malloc(sizeof(char)*BUF_SIZE);
    int msg_len=1;

    //child process
    // call the thread function
    pthread_t tid;
    rec_buf_cont *rec1 = malloc(sizeof(rec_buf_cont));
    rec1->iter = iter;
    rec1->buffer_res = buffer_res;
    rec1->buffer = buffer;
    rec1->len = -1;
    rec1->message_entry = message_entry;
    rec1->textview=textview;
    rec1->scrollview=scrollview;
    rec1->btn_send = btn_send;

    // call thread to receive messages
    pthread_create(&tid, NULL, myThreadFun, (void *)rec1);

    g_signal_connect(G_OBJECT(btn_send), "clicked", G_CALLBACK(send_button_clicked), (gpointer) rec1);
    g_signal_connect(G_OBJECT(window), "destroy", myQuitFunction, NULL);

    g_object_unref(icon);

    gtk_main();

    return 0;
}