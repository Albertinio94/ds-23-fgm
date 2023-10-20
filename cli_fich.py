#!/usr/bin/env python3

import socket, sys, os
import szasar

FILES_SERVER = 'localhost'
INOTIFY_HOST = 'localhost'
FILES_PORT = 6012
INOTIFY_PORT = 7778
Options = ( "Lista de ficheros", "Bajar fichero", "Subir fichero", "Borrar fichero", "Salir" )
List, Download, Upload, Delete, Exit = range( 1, 6 )
ER_MSG = (
	"Correcto.",
	"Comando desconocido o inesperado.",
	"Usuario desconocido.",
	"Clave de paso o password incorrecto.",
	"Error al crear la lista de ficheros.",
	"El fichero no existe.",
	"Error al bajar el fichero.",
	"Un usuario anonimo no tiene permisos para esta operacion.",
	"El fichero es demasiado grande.",
	"Error al preparar el fichero para subirlo.",
	"Error al subir el fichero.",
	"Error al borrar el fichero." )


def sendOK( s, params="" ):
	s.sendall( ("OK{}\r\n".format( params )).encode( "ascii" ) )

def sendER( s, code=1 ):
	s.sendall( ("ER{}\r\n".format( code )).encode( "ascii" ) )

def listen_to_inotify(dialog_socket):

	while True:
		msg = dialog_socket.recv(1024).decode('ascii')
		try:
			selected = int(msg[0])
			sendOK(dialog_socket)
		except:
			print( "Recibido %x", msg.encode('ascii') )
			sendER(dialog_socket)
			continue
		if 0 < selected <= len( Options ):
			return selected, msg[1:]
		else:
			print( "Opción no válida." )

def iserror( message ):
	if( message.startswith( "ER" ) ):
		code = int( message[2:] )
		print( ER_MSG[code] )
		return True
	else:
		return False

def int2bytes( n ):
	if n < 1 << 10:
		return str(n) + " B  "
	elif n < 1 << 20:
		return str(round( n / (1 << 10) ) ) + " KiB"
	elif n < 1 << 30:
		return str(round( n / (1 << 20) ) ) + " MiB"
	else:
		return str(round( n / (1 << 30) ) ) + " GiB"



if __name__ == "__main__":
	if len( sys.argv ) > 3:
		print( "Uso: {} [<servidor> [<puerto>]]".format( sys.argv[0] ) )
		exit( 2 )

	if len( sys.argv ) >= 2:
		FILES_SERVER = sys.argv[1]
	if len( sys.argv ) == 3:
		FILES_PORT = int( sys.argv[2])

	inotify_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	inotify_socket.bind((INOTIFY_HOST, INOTIFY_PORT))

	inotify_socket.listen(1)

	inotify_dialog, inotify_address = inotify_socket.accept()

	#############inotify_socket.close()


	files_socket = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
	files_socket.connect( (FILES_SERVER, FILES_PORT) )

	while True:
		user = input( "Introduce el nombre de usuario: " )
		message = "{}{}\r\n".format( szasar.Command.User, user )
		files_socket.sendall( message.encode( "ascii" ) )
		message = szasar.recvline( files_socket ).decode( "ascii" )
		if iserror( message ):
			continue

		password = input( "Introduce la contraseña: " )
		message = "{}{}\r\n".format( szasar.Command.Password, password )
		files_socket.sendall( message.encode( "ascii" ) )
		message = szasar.recvline( files_socket ).decode( "ascii" )
		if not iserror( message ):
			break

	sendOK(inotify_dialog)

	while True:
		option, additional_data = listen_to_inotify(inotify_dialog)

		if option == Upload:
			filename = additional_data.split('/')[-1]
			abs_filename = os.path.abspath(additional_data)
			print(filename)
			print(abs_filename)
			try:
				filesize = os.path.getsize( abs_filename )
				with open( abs_filename, "rb" ) as f:
					filedata = f.read()
			except:
				print( "No se ha podido acceder al fichero {}.".format( filename ) )
				continue

			message = "{}{}?{}\r\n".format( szasar.Command.Upload, filename, filesize )
			files_socket.sendall( message.encode( "ascii" ) )
			message = szasar.recvline( files_socket ).decode( "ascii" )
			if iserror( message ):
				continue

			message = "{}\r\n".format( szasar.Command.Upload2 )
			files_socket.sendall( message.encode( "ascii" ) )
			files_socket.sendall( filedata )
			message = szasar.recvline( files_socket ).decode( "ascii" )
			if not iserror( message ):
				print( "El fichero {} se ha enviado correctamente.".format( filename ) )

		elif option == Delete:
			filename = additional_data.split('/')[-1]
			message = "{}{}\r\n".format( szasar.Command.Delete, filename )
			files_socket.sendall( message.encode( "ascii" ) )
			message = szasar.recvline( files_socket ).decode( "ascii" )
			if not iserror( message ):
				print( "El fichero {} se ha borrado correctamente.".format( filename ) )

		elif option == Exit:
			message = "{}\r\n".format( szasar.Command.Exit )
			files_socket.sendall( message.encode( "ascii" ) )
			message = szasar.recvline( files_socket ).decode( "ascii" )
			break
	files_socket.close()
	inotify_dialog.close()
	inotify_socket.close()
	print("El programa se ha cerrado con exito.")