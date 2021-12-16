#include<iostream>
#include<fstream>
#include<string>
#include<algorithm>
#include<qtextstream.h>
#include<qfile.h>
#include<qdir.h>
#include<map>

using namespace std;

bool compareTo(const QList<int>& list1, const QList<int>& list2) {
	if (list1.at(list1.length() - 2) == list2.at(list2.length() - 2)) {
		return list1.at(list1.length() - 1) < list2.at(list2.length() - 1);
	}
	else {
		return list1.at(list1.length() - 2) < list2.at(list2.length() - 2);
	}
}

string readHeader(QTextStream& in) {
	QString line = in.readLine();
	if (line.contains("GAMBIT")) {
		return "Gambit";
	}
	if (line.contains("ANSYS")) {
		return "ANSYS";
	}
	if (line.contains("Fluent")) {
		return "Fluent";
	}
	return "error";
}

void writeHeader(ofstream& out, string type, string notes) {
	out << "/*--------------------------------*- C++ -*----------------------------------*\\\n"
		"  =========                 |\n"
		"  \\\\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox\n"
		"   \\\\    /   O peration     | Website:  https://openfoam.org\n"
		"    \\\\  /    A nd           | Version:  8\n"
		"     \\\\/     M anipulation  |\n"
		"\\*---------------------------------------------------------------------------*/\n"
		"FoamFile\n" 
		"{\n" 
		"    version     2.0;\n" 
		"    format      ascii;\n" 
		"    class       vectorField;\n"
		+notes+
		"    location    \"constant/polyMesh\";\n" 
		"    object      "+type+";\n" 
		"}\n" 
		"// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n" << endl;
}

void writeBoundary(ofstream& outb, string name, int nFaces, int startFaces) {
	string type = name.compare("wall") ? "patch" : "wall";
	string str = name.compare("wall") ?  "" : "        inGroups        List<word> 1(wall);\n";
	outb << "    "+name+"\n"
		"    {\n"
		"        type            "+type+";\n"
		+str+
		"        nFaces          "<<nFaces<<";\n"
		"        startFace       "<<startFaces<<";\n"
		"    }" << endl;
}


void readPoints10(QTextStream& in, ofstream& out, QString line, string meshType) {
	
	int firstIndex = 0;
	int lastIndex = 0;
	QStringList sList = line.split(" ");
	
	firstIndex = sList.at(2).toInt(0, 16);
	lastIndex = sList.at(3).toInt(0, 16);
	if (meshType.compare("Fluent") == 0) {//ICEM转化的mesh多上行(
		in.readLine();
	}
	for (int i = firstIndex; i <= lastIndex; ++i) {
		line = in.readLine();
		sList = line.simplified().split(" ");
		out << "(" << sList.at(0).toStdString() << " " << sList.at(1).toStdString() << " " << sList.at(2).toStdString() << ")" << endl;
	}
}

void readFaces13(int start, int end, int faceType, bool isInternal, string meshType, QList<QList<int>>& faces, QTextStream& in) {
	if (!isInternal) {
		QList<QList<int>> aBoundary ;
		QStringList sList;
		for (int i = start; i <= end; ++i) {
			sList = in.readLine().simplified().split(" ");
			QList<int> list;//list里放的是  fnp p1 p2...pn ownerid neighbourid
			int fnp = faceType != 0 ? faceType : sList.at(0).toInt();//一个面有多少个点。
			list.append(fnp);
			int j = faceType != 0 ? 0 : 1;
			int jend = faceType != 0 ? fnp - 1 : fnp;
			for (; j <= jend; ++j) {
				list.append(sList.at(j).toInt(0, 16) - 1);
			}
			list.append(sList.at(sList.length() - 2).toInt(0, 16) - 1);
			list.append(0);
			aBoundary.append(list);
		}
		sort(aBoundary.begin(), aBoundary.end(),compareTo);
		faces.append(aBoundary);	
	}
	else {
		QStringList sList;
		for (int i = start; i <= end; ++i) {
			sList = in.readLine().simplified().split(" ");
			QList<int> list; // list里放的是  fnp p1 p2...pn ownerid neighbourid
			int fnp = faceType != 0 ? faceType : sList.at(0).toInt();//一个面有多少个点。
			list.append(fnp);
			int j = faceType != 0 ? 0 : 1;
			int jend = faceType != 0 ? fnp - 1 : fnp;
			for (; j <= jend; ++j) {
				list.append(sList.at(j).toInt(0, 16) - 1);
			}
			//这里Gambit转的mesh和Ansys、ICEM转的mesh相反。
			if (meshType.compare("Gambit") == 0) {
				list.append(sList.at(sList.length() - 1).toInt(0, 16) - 1);
				list.append(sList.at(sList.length() - 2).toInt(0, 16) - 1);
			}
			else {
				list.append(sList.at(sList.length() - 2).toInt(0, 16) - 1);
				list.append(sList.at(sList.length() - 1).toInt(0, 16) - 1);
			}
			faces.append(list);
		}
	}
}



void fluentMeshToFoam(QString modelPath, string workPath) {
	QFile infile(modelPath);
	QTextStream in(&infile);
	if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return;
	}
	//QFile filep(workPath + "/points");
	//if (!filep.open(QFile::WriteOnly | QFile::Truncate)) {
	//	return;
	//}
	//QTextStream outp(&filep);
	//outp << "fuck" << endl;

	QDir dir(QString::fromStdString(workPath) + "/constant/polyMesh");
	if (!dir.exists()) {
		dir.mkpath(QString::fromStdString(workPath) + "/constant/polyMesh");
	}

	ofstream outp(workPath + "/constant/polyMesh/points",ios::out);
	ofstream outf(workPath + "/constant/polyMesh/faces", ios::out);
	ofstream outo(workPath + "/constant/polyMesh/owner",ios::out);
	ofstream outn(workPath + "/constant/polyMesh/neighbour",ios::out);
	ofstream outb(workPath + "/constant/polyMesh/boundary",ios::out);

	int np = 0; //点的总数量
	int nf = 0;//面的总数量
	int nc = 0;//单元的总数量
	int nInternalFace = 0;//内部面的总数量 
	int accumulateBoundaryFace = 0;//累计边界面的数量 
	bool hasWritePatchNum = false;//是否写边界面数量

	QList<QList<int>> internalFace;//内部面集合
	QList<QList<int>> boundaryFace;//边界面集合
	QHash<int, int> map;//key:zoneid  value:nFaces

	QString line;
	QStringList sList;
	string meshType = readHeader(in);//网格类型

	while (!in.atEnd()) {
		line = in.readLine();
		if (line.endsWith(")(")) {
			line = line.replace(")", " ").replace("(", " ").simplified();
			sList = line.split(" ");
			if (sList.at(0) == "10") {//读取点
				readPoints10(in,outp,line,meshType);
			}
			else if (sList.at(0) == "13") {//读取面
				int zoneid = sList.at(1).toInt(0, 16);
				int firstIndex = sList.at(2).toInt(0, 16);
				int lastIndex = sList.at(3).toInt(0, 16);
				int bcType = sList.at(4).toInt(0, 16);
				int faceType = sList.at(5).toInt(0, 16);
				int nFaces = lastIndex - firstIndex + 1;
				if (bcType == 2) {//内部面
					nInternalFace += nFaces;
					readFaces13(firstIndex - 1, lastIndex - 1, faceType, true, meshType, internalFace, in);
				}
				else {
					readFaces13(firstIndex - 1, lastIndex - 1, faceType, false, meshType, boundaryFace, in);
					map.insert(zoneid, nFaces);
				}
			}
		}
		else if (line.startsWith("(") && line.endsWith("))")) {
			line = line.replace(")", " ").replace("(", " ").simplified();
			sList = line.split(" ");
			if (sList.at(0) == "10" && sList.at(1) == "0") {
				np = sList.at(3).toInt(0, 16);
				writeHeader(outp, "points", "");//写点文件头
				outp << np << "\n" << "(" << endl;
			}
			else if (sList.at(0) == "12" && sList.at(1) == "0") {
				nc = sList.at(3).toInt(0, 16);//记录总单元数
			}
			else if (sList.at(0) == "13" && sList.at(1) == "0") {
				nf = sList.at(3).toInt(0, 16);//记录总面数
			}
			else if (sList.at(0) == "45" || sList.at(0) == "39") {
				if (!hasWritePatchNum) {//写边界头文件
					writeHeader(outb, "boundary", "");
					outb << map.size() << "\n" << "(" << endl;
					hasWritePatchNum = true;
				}
				if (map.find(sList.at(1).toInt()) != map.end()) {
					int nFaces = *map.find(sList.at(1).toInt());//这个边界块有多少个面
					string name = sList.at(3).toStdString();//边界的命名
					writeBoundary(outb, name, nFaces, nInternalFace + accumulateBoundaryFace);
					accumulateBoundaryFace += nFaces;
				}
			}
		}
	}

	infile.close();
	outp << ")" << endl;
	outb << ")" << endl;
	outp.close();
	outb.close();

	sort(internalFace.begin(), internalFace.end(), compareTo);//将内部面先按owner大小排序,owner相同则按neighbour。

	writeHeader(outf, "faces", "");
	string note = "    note        \"nPoints:" + to_string(np) + " nCells:" + to_string(nc) + " nFaces: " + to_string(nf) + " nInternalFaces:" + to_string(nInternalFace) + "\";\n";
	writeHeader(outo, "owner", note);
	writeHeader(outn, "neighbour", note);
	outf << nf << "\n" << "(" << endl;
	outo << nf << "\n" << "(" << endl;
	outn << nInternalFace << "\n" << "(" << endl;
	QList<int> list;

	for (int i = 0; i < internalFace.size(); ++i) {
		list = internalFace.at(i);
		int fnp = list.at(0);
		int size = list.size();
		outf << fnp << "(";
		for (int j = 1; j <= fnp; ++j) {
			outf << list.at(j) << " ";
		}
		outf << ")" << endl;
		outo << list.at(size - 2) << endl;
		outn << list.at(size - 1) << endl;
	}

	for (int i = 0; i < boundaryFace.size(); ++i) {
		list = boundaryFace.at(i);
		int fnp = list.at(0);
		int size = list.size();
		outf << fnp << "(";
		for (int j = 1; j <= fnp; ++j) {
			outf << list.at(j) << " ";
		}
		outf << ")" << endl;
		outo << list.at(size - 2) << endl;
	}

	outf << ")" << endl;
	outo << ")" << endl;
	outn << ")" << endl;

	outf.close();
	outo.close();
	outn.close();
}

int main(int argc, char *argv[])
{
	fluentMeshToFoam("xxx", "xxx");
	system("pause");
  	return 0;
}
