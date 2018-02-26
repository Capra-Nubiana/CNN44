#include "gordon.NetParameter.pb.h"
#include "gordon_cnn.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <json/json.h>  //���ð�װjson�ļ����������
using namespace std;
using namespace arma;
//���ߣ�gordon
/*
ħ���ĸ���ܶ����͵��ļ�������ʼ�ļ����ֽڵ������ǹ̶��ģ�����������䣬���Ǳ�����ˣ���
�����⼸���ֽڵ����ݾͿ���ȷ���ļ����ͣ�����⼸���ֽڵ����ݱ���Ϊħ�� (magic number)��
��˴洢��С�˴洢��һ����������д��ʮ����ϸ��������˵����˴洢�����˵�����˼ά��С�˴洢�������������
С�ˣ��ϸߵ���Ч�ֽڴ���ڽϸߵĵĴ洢����ַ���ϵ͵���Ч�ֽڴ���ڽϵ͵Ĵ洢����ַ��
��ˣ��ϸߵ���Ч�ֽڴ���ڽϵ͵Ĵ洢����ַ���ϵ͵���Ч�ֽڴ���ڽϸߵĴ洢����ַ��
mnistԭʼ�����ļ���32λ������ֵ�Ǵ�˴洢��C/C++������С�˴洢�����Զ�ȡ���ݵ�ʱ����Ҫ������д�С��ת����
������c++����mnist�ɲο����ͣ�
http://blog.csdn.net/lhanchao/article/details/53503497
http://blog.csdn.net/sheng_ai/article/details/23267039
����ĸ�ʽ�����ܼ����� ��
http://yann.lecun.com/exdb/mnist/
ע�⣺ֻ���ļ�ͷ�ĸ���������Ҫ��С��ת���������60000����Ч��������Ҫ��
*/

int ReverseInt(int i)  ////�Ѵ������ת��Ϊ���ǳ��õ�С������ ����С��ģʽ��ԭ�򣬿���Lecun�������ݼ��õĴ�С�˸����ǵĻ�����һ����
{
	unsigned char ch1, ch2, ch3, ch4;  //һ��int��4��char
	ch1 = i & 255;
	ch2 = (i >> 8) & 255;
	ch3 = (i >> 16) & 255;
	ch4 = (i >> 24) & 255;
	return ((int)ch1 << 24) + ((int)ch2 << 16) + ((int)ch3 << 8) + ch4;
}

void ReadMnistLabel(string path, shared_ptr<Blob>& label) {
	ifstream file(path, ios::binary);
	if (file.is_open())
	{
		//1.���ļ��л�֪ħ�����֣�ͼƬ����
		int magic_number = 0;
		int number_of_images = 0;
		file.read((char*)&magic_number, sizeof(magic_number));
		magic_number = ReverseInt(magic_number);
		cout << "magic_number=" << magic_number << endl;
		file.read((char*)&number_of_images, sizeof(number_of_images));
		number_of_images = ReverseInt(number_of_images);
		cout << "number_of_Labels=" << number_of_images << endl;
		//2.�����б�ǩתΪBlob�洢������д����ʶ��0~9��
		for (int i = 0; i<number_of_images; ++i)
		{
			unsigned char temp = 0;
			file.read((char*)&temp, sizeof(temp));
			//cout << "Label=" << (int)temp << endl;
			//����Blob����Blobʵ������һ��vector<cube>������cube(height����, width����, chanel����)��
			//���� (*label)[i](0, 0, (int)temp) ��ʾ��i��cube��(0,0)�㴦��tempͨ����ע�⣺��main��ʼ���洢label��cube��1��1��10ͨ������ȣ���
			(*label)[i](0, 0, (int)temp) = 1;  //��one hot����ʽ�ı�ǩ��ʾ��������0~9�ĸ���1���ǩΪ����//ע�⣺cube��ʽΪ(h, w, c)
		}
	}
	else {
		cout << "no label file found :-(" << endl;
	}
}

void ReadMnistData(string path, shared_ptr<Blob>& image)
{
	ifstream file(path, ios::binary);  //����·���б��ļ���json�ļ���
	if (file.is_open())
	{
		//mnistԭʼ�����ļ���32λ������ֵ�Ǵ�˴洢��C/C++������С�˴洢�����Զ�ȡ���ݵ�ʱ����Ҫ������д�С��ת��!!!!
		//1.���ļ��л�֪ħ�����֣�һ�㶼���𵽱�ʶ�����ã�û������ʵ���Ե��ô�����ͼƬ������ͼƬ�����Ϣ
		int magic_number = 0;
		int number_of_images = 0;
		int n_rows = 0;
		int n_cols = 0;
		file.read((char*)&magic_number, sizeof(magic_number));
		magic_number = ReverseInt(magic_number);  //�ߵ��ֽڵ���
		cout << "magic_number=" << magic_number << endl;
		file.read((char*)&number_of_images, sizeof(number_of_images));
		number_of_images = ReverseInt(number_of_images);
		cout << "number_of_images=" << number_of_images << endl;
		file.read((char*)&n_rows, sizeof(n_rows));
		n_rows = ReverseInt(n_rows);
		cout << "n_rows=" << n_rows << endl;
		file.read((char*)&n_cols, sizeof(n_cols));
		n_cols = ReverseInt(n_cols);
		cout << "n_cols=" << n_cols << endl;

		//2.��ͼƬתΪBlob�洢��
		for (int i = 0; i<number_of_images; ++i)  //��������ͼƬ
		{
			for (int r = 0; r<n_rows; ++r)   //������
			{
				for (int c = 0; c<n_cols; ++c)   //������
				{
					unsigned char temp = 0;
					file.read((char*)&temp, sizeof(temp));      //����һ������ֵ��		
					//���ӻ�����
					//double tmp = (double)temp / 255;
					//if (tmp != 0)tmp = 1;
					//cout << tmp << " ";
					//if (c == 27)cout << endl;		 //��һ��������5������0��4��1��9��
					//��һ�������Blob����Blobʵ������һ��vector<cube>������cube(height����, width����, chanel����)��
					(*image)[i](r, c, 0) = (double)temp / 255;    //ע�⣺cube��ʽΪ(h, w, c)
				}
			}
		}
	}
	else
	{
		cout << "no data file found :-(" << endl;
	}
}

void trainMnist(shared_ptr<Blob>& X, shared_ptr<Blob>& Y, string config)
{
	NetParam param;
	param.readNetParam(config);  //��ȡ����ṹ���ò�����configΪjson�����ļ�����·����

	shared_ptr<Blob> X_train(new Blob(X->subBlob(0, 59000))); //ѵ��������
	shared_ptr<Blob> Y_train(new Blob(Y->subBlob(0, 59000))); //ѵ������ǩ
	shared_ptr<Blob> X_val(new Blob(X->subBlob(59000, 60000)));//��֤������
	shared_ptr<Blob> Y_val(new Blob(Y->subBlob(59000, 60000)));//��֤����ǩ
	vector<shared_ptr<Blob>> XX{ X_train, X_val }; //���ݼ�
	vector<shared_ptr<Blob>> YY{ Y_train, Y_val }; //��ǩ��
	vector<std::string> ltypes_ = param.ltypes;
	vector<std::string> layers_ = param.layers;
	for (int i = 0; i < ltypes_.size(); ++i)
	{
		cout << "ltype = " << ltypes_[i] << endl;
	}
	for (int i = 0; i < layers_.size(); ++i)
	{
		cout << "layer = " << layers_[i] << endl;
	}
	Net inst;
	//��ʼ������ṹ��
	inst.initNet(param, XX, YY); 
	////////inst.testNet(param);  //���������磨��㣩���ݶȼ���
	//��ʼѵ����
	inst.train(param);   //�����ԵĻ�Ҳ�����������������������trainNet������mode=forward�������ƾͿ��ԣ������Լ�ר��дһ�����Եĺ��������ѣ�
	
}


int main(int argc, char** argv)
{

	//shared_ptr<Blob> images(new Blob(10000, 1, 28, 28));  //����һ��Blob��cube����10000��ÿ��cubeΪͨ��1��28��28��
	//ReadMnistData("mnist_data/test/t10k-images.idx3-ubyte", images);  //��ȡdata
	//vector<cube> lists = images->get_data();
	//for (int i = 0; i<3; ++i)  //��������ͼƬ
	//{
	//	for (int r = 0; r<28; ++r)   //������
	//	{
	//		for (int c = 0; c<28; ++c)   //������
	//		{
	//			double tmp = lists[i](r, c, 0);  //��ӡÿһ������
	//			if (tmp != 0)tmp = 1;
	//			cout << tmp << " ";
	//			if (c == 27)cout << endl;		 //��һ��������5������0��4��1��9��
	//		}
	//	}
	//}
	//-------------------------------------------------------------------------------------

	shared_ptr<Blob> images(new Blob(60000, 1, 28, 28));  //����һ��Blob��cube����10000��ÿ��cubeΪͨ��1��28��28��
	shared_ptr<Blob> labels(new Blob(60000, 10, 1, 1, TZEROS));//����һ��Blob��cube����10000��ÿ��cubeΪͨ������ȣ�10��1��1��  [10000,10,1,1]
	ReadMnistData("mnist_data/train/train-images.idx3-ubyte", images);  //��ȡdata
	ReadMnistLabel("mnist_data/train/train-labels.idx1-ubyte", labels);   //��ȡlabel
	trainMnist(images, labels, "mnist.json");  //��ʼѵ��

	 return 0;
}




