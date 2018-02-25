#include "myNet.hpp"
using namespace std;

void NetParam::readNetParam(std::string file) 
{
	std::ifstream ifs;
	ifs.open(file);
	assert(ifs.is_open());
	Json::Reader reader;
	Json::Value value;
	if (reader.parse(ifs, value))
	{
		if (!value["train"].isNull()) 
		{
			auto &tparam = value["train"];
			this->lr = tparam["learning rate"].asDouble(); //������Double���ʹ��
			this->lr_decay = tparam["lr decay"].asDouble();
			this->update = tparam["update method"].asString();//������String���ʹ��
			this->momentum = tparam["momentum parameter"].asDouble();
			this->num_epochs = tparam["num epochs"].asInt();//������Int���ʹ��
			this->use_batch = tparam["use batch"].asBool();//������Bool���ʹ��
			this->batch_size = tparam["batch size"].asInt();
			this->reg = tparam["reg"].asDouble();
			this->acc_frequence = tparam["acc frequence"].asInt();
			this->acc_update_lr = tparam["frequence update"].asBool();
			this->snap_shot = tparam["snapshot"].asBool();
			this->snapshot_interval = tparam["snapshot interval"].asInt();
			this->fine_tune = tparam["fine tune"].asBool();
			this->preTrainModel = tparam["pre train model"].asString();//������String���ʹ��
		}
		if (!value["net"].isNull()) 
		{
			auto &nparam = value["net"];
			for (int i = 0; i < (int)nparam.size(); ++i) 
			{
				auto &ii = nparam[i];
				this->layers.push_back(ii["name"].asString());
				this->ltypes.push_back(ii["type"].asString());
				if (ii["type"].asString() == "Conv")
				{
					int num = ii["kernel num"].asInt();
					int width = ii["kernel width"].asInt();
					int height = ii["kernel height"].asInt();
					int pad = ii["pad"].asInt();
					int stride = ii["stride"].asInt();
					this->params[ii["name"].asString()].setConvParam(stride, pad, width, height, num);
				}
				if (ii["type"].asString() == "Pool") 
				{
					int stride = ii["stride"].asInt();
					int width = ii["kernel width"].asInt();
					int height = ii["kernel height"].asInt();
					this->params[ii["name"].asString()].setPoolParam(stride, width, height);
				}
				if (ii["type"].asString() == "Fc") 
				{
					int num = ii["kernel num"].asInt();
					this->params[ii["name"].asString()].fc_kernels = num;
				}
			}
		}
	}
}



	void Net::trainNet(shared_ptr<Blob>& X,   //X_batch��mini-batch��
		shared_ptr<Blob>& Y,   //Y_batch��mini-batch��
		NetParam& param,
		std::string mode)  //mode���ڿ�������ѵ���������ڲ��ԣ�
	{
		/*! fill X, Y */
		data_[layers_[0]][0] = X;         //������mini-batch��������������Blob�У���A_prev��
		data_[layers_.back()][1] = Y;   //������mini-batch��labels��������ĩ��ı�ǩBlob�У���Y����data_[layers_.back()][0] Ϊ����Blob��

		// debug
		Blob pb, pd;

		/* step1. forward ��ʼǰ�򴫲�������*/
		int n = ltype_.size();  //�����͸�����=����������
		for (int i = 0; i < n - 1; ++i)   //������
		{
			std::string ltype = ltype_[i];       //��ǰ������
			std::string lname = layers_[i];   //��ǰ�������������͵���������һ��Ӧ�ģ�
			shared_ptr<Blob> out;             //����һ�����Blob������ò�ǰ��������õ����
			if (ltype == "Conv")
			{
				int tF = param.params[lname].conv_kernels;   //����˸������ɲ����������õ���
				int tC = data_[lname][0]->get_C();                    //�������ȣ�=����Blob��ȣ�
				int tH = param.params[lname].conv_height;   //����˸�
				int tW = param.params[lname].conv_width;    //����˿�  
				/*--------------------  ��ʼ�������W��ƫ��b��Ҳ�͵�һ��forwardʱ��ִ�У�  ----------------------*/
				if (!data_[lname][1])   //����conv��ľ����BlobΪ�գ��򴴽���������Ԥѵ��Ȩ�ؼ��أ��򲻻�ִ�г�ʼ����
				{
					cout << "------ Init weights  with Gaussian ------" << endl;
					data_[lname][1].reset(new Blob(tF, tC, tH, tW, TRANDN)); //����Blob����Ϊ��tF, tC, tH, tW��
					(*data_[lname][1]) *= 1e-2;  //��ʼ��������˳���0.01������Ȩֵ��һ����0.01��
				}
				if (!data_[lname][2])    //����conv���ƫ��BlobΪ�գ��򴴽���������Ԥѵ��bias���أ��򲻻�ִ�г�ʼ����
				{
					cout << "------ Init bias with Gaussian ------" << endl;
					data_[lname][2].reset(new Blob(tF, 1, 1, 1, TRANDN)); //����Blob����Ϊ��tF, 1, 1, 1��
					(*data_[lname][2]) *= 1e-1;  // ��ʼ����bias����0.01������bias��һ����0.1��
				}
				/*------  �þ���㿪ʼ��ǰ�򴫲���  -------*/
				ConvLayer::forward(data_[lname], out, param.params[lname]);
			}
			if (ltype == "Pool")
			{
				PoolLayer::forward(data_[lname], out, param.params[lname]);
				pb = *data_[lname][0];
			}
			if (ltype == "Fc")
			{
				int tF = param.params[lname].fc_kernels;
				int tC = data_[lname][0]->get_C();
				int tH = data_[lname][0]->get_H();
				int tW = data_[lname][0]->get_W();
				if (!data_[lname][1])
				{
					data_[lname][1].reset(new Blob(tF, tC, tH, tW, TRANDN));
					(*data_[lname][1]) *= 1e-2;
				}
				if (!data_[lname][2])
				{
					data_[lname][2].reset(new Blob(tF, 1, 1, 1, TRANDN));
					(*data_[lname][2]) *= 1e-1;
				}
				AffineLayer::forward(data_[lname], out);
			}
			if (ltype == "Relu")
			{
				ReluLayer::forward(data_[lname], out);
			}
			if (ltype == "Dropout")
			{
				DropoutLayer::forward(data_[lname], out, param.params[lname]);
			}
			//------------------------------------------------------
			data_[layers_[i + 1]][0] = out;   //���ò����Blob������һ�������Blob������ִ��ǰ����㣬ֱ��Softmax���ǰһ�㣡
			//------------------------------------------------------
		}

		/* step2. ��ĩ��softmax��ǰ�򴫲��ͼ�����ۣ�*/
		std::string loss_type = ltype_.back(); //��ĩ��Ĳ�����
		shared_ptr<Blob> dout;  //��������źţ�������ʧ�ݶȣ��������򴫲��ģ�
		if (loss_type == "SVM")
			SVMLossLayer::go(data_[layers_.back()], loss_, dout);
		if (loss_type == "Softmax")
			SoftmaxLossLayer::go(data_[layers_.back()], loss_, dout);  //��ĩ�㣺Softmax���ǰ�򴫲������Եõ�����forward��ʧֵloss_����ʧ�ݶ�dout
		grads_[layers_.back()][0] = dout;

		loss_history_.push_back(loss_); //���ӽ�loss�����У����Դ�ӡ�����Ҳ���԰�ÿ���������ڵ���ʧ�����ͼ��

		if (mode == "forward")  //���������ǰ�򴫲��������ԣ���ѵ����������ǰ�˳���
			return;

		/* step3. backward ��ʼ���򴫲���*/
		for (int i = n - 2; i >= 0; --i)  //����ĩ��֮ǰ��һ�㣨����ȥsoftmax������ȫ���Ӳ㿪ʼ��㷴�򴫲�
		{
			std::string ltype = ltype_[i];
			std::string lname = layers_[i];
			if (ltype == "Conv")
			{
				//���������Relu��õ�����ʧ�ݶȣ�����źţ� -- conv��A_prev��W, b  --  conv�㷴�����õ���dA��dW, db -- conv��kernel��ߺͲ�������    
				ConvLayer::backward(grads_[layers_[i + 1]][0], data_[lname], grads_[lname], param.params[lname]);
			}
			if (ltype == "Pool")
			{
				//���������fc��õ�����ʧ�ݶȣ�����źţ� -- pool��A_prev��W, b  --  pool�㷴�����õ���dA��dW, db -- pool���ߺͲ�������    
				PoolLayer::backward(grads_[layers_[i + 1]][0], data_[lname], grads_[lname], param.params[lname]);
			}
			if (ltype == "Fc")
			{
				//���������softmax�õ���dout ������źţ�-- ȫ���Ӳ�A_prev��W, b -- ȫ���Ӳ㷴�����õ���dA��dW, db
				AffineLayer::backward(grads_[layers_[i + 1]][0], data_[lname], grads_[lname]);
			}
			if (ltype == "Relu")
			{
				//���������pool��õ�����ʧ�ݶ� ������źţ�-- Relu��A_prev��W, b -- Relu�㷴�����õ���dA��dW, db
				ReluLayer::backward(grads_[layers_[i + 1]][0], data_[lname], grads_[lname]);
			}
		}

		// regularition ���򻯣�
		double reg_loss = 0;
		for (auto i : layers_)  //�����㣨c++11�﷨��
		{
			if (grads_[i][1])  //�ò�Ȩֵ�ݶȲ�Ϊ��
			{
				// it's ok?                       //-----------gordon���ţ����������ע��ɶ��˼���ѵ���Ҳ��ȷ����δ����в��У���
				Blob reg_data = param.reg * (*data_[i][1]);
				(*grads_[i][1]) = (*grads_[i][1]) + reg_data;
				reg_loss += data_[i][1]->sum();
			}
		}
		reg_loss *= param.reg * 0.5;
		loss_ = loss_ + reg_loss;

		return;
	}

	//void Net::testNet(NetParam& param) 
	//{
	//	shared_ptr<Blob> X_batch(new Blob(X_train_->subBlob(0, 1)));
	//	shared_ptr<Blob> Y_batch(new Blob(Y_train_->subBlob(0, 1)));
	//	trainNet(X_batch, Y_batch, param);
	//	cout << "BEGIN TEST LAYERS" << endl;
	//	for (int i = 0; i < (int)layers_.size(); ++i)  //�����㣬������ݶȼ���
	//	{
	//		testLayer(param, i);  //�����ݶȼ��飬����������������ڼ���
	//		printf("\n");
	//	}
	//}

	void Net::ListParam(const gordon::NetParameter& net_param)
	{
		cout << "net_param.dict_size() = " << net_param.dict_size() << endl;
		for (int i = 0; i < net_param.dict_size(); i++)  //���������ֵ䣨ÿ����һ��ģ�Ͳ������գ��ͻ��һ���ֵ䣩
		{
			const gordon::LayerDictionary& dict = net_param.dict(i);
			for (int j = 0; j < dict.blobs_size(); j++)  //���������ֵ��е����в���Blob
			{
				const gordon::LayerDictionary::ParamBlob& blob = dict.blobs(j);
				cout << "LayerName = " << blob.lname() << "     LayerType = " << blob.type() << endl;
				cout << "Blob(N,C,H,W) = ��" << blob.cube_num() << "," << blob.cube_ch() << "," << blob.cube_size() << "," << blob.cube_size() << "��" << endl;
				int number = 0;
				shared_ptr<Blob> param_blob(new Blob(blob.cube_num(), blob.cube_ch(), blob.cube_size(), blob.cube_size()));
				//cout << "-----------------" << endl;
				for (int i = 0; i<blob.cube_num(); ++i)  //������Blob���о����
				{
					for (int c = 0; c < blob.cube_ch(); ++c)  //�������
					{
						for (int h = 0; h<blob.cube_size(); ++h)   //������
						{
							for (int w = 0; w<blob.cube_size(); ++w)   //������
							{
								const gordon::LayerDictionary::ParamBlob::ParamValue& param_val = blob.param_val(number);
								//cout << param_val.val() << " ";
								(*param_blob)[i](h, w, c) = param_val.val();
								number++;
							}
							//cout << endl;
						}
						//cout << "---------------" << endl;
					}
					//cout << "= = = = = = = = = = = = = = = = = = = = = = = " << endl;
				}
				//cout << "param number = " << number << endl;
				//����Ҫʵ�֣���(*param_blob)���ݱ��������ӽ�data_�����κ����ƶ���initNet()�л᲻��ȽϺã�
				string layer_name = blob.lname();
				if (blob.type()==0)
					data_[layer_name][1] = param_blob;  //Ȩֵ
				else
					data_[layer_name][2] = param_blob;  //ƫ��
			}
			cout << "-----------------" << endl;
		}
		cout << endl << "/////////////////////////////// ����data_ /////////////////////////////////" << endl << endl;
		for (auto lname : layers_)  //�����㣨c++11�﷨��
		{
			for (int j = 1; j <= 2; ++j)   //jΪ1ʱ���¸ò�ȨֵW,  jΪ2ʱ���¸ò�ƫ��b
			{
				if (!data_[lname][1] || !data_[lname][2])   //���ò�����Blob��Ȩ�غ�bias��һ��Ϊ�գ���ʱ����Ϊrelu���pool��û�в����ģ�����Ҫ������
				{
					continue;  //��������ѭ��������ִ��ѭ����ע�ⲻ��break����ѭ����
				}
				int number = 0;
				int blob_num = data_[lname][j]->get_N();  //��Blob��cube������ˣ�����
				int blob_ch = data_[lname][j]->get_C();     //cube�����
				int blob_height = data_[lname][j]->get_H();  //cube�ĸ�
				int blob_width = data_[lname][j]->get_W();  //cube�Ŀ�
				if (j == 1)
					cout << "LayerName = " << lname << "     LayerType = 0 (weight)" << endl;
				else
					cout << "LayerName = " << lname << "     LayerType = 1 (bias)" << endl;
				cout << "Blob(N,C,H,W) = ��" << blob_num << "," << blob_ch << "," << blob_height << "," << blob_width << "��" << endl;
				vector<cube> lists = data_[lname][j]->get_data();  //���weight
				assert(lists.size() == blob_num);
				cout << "-------------------------------------" << endl;
				for (int i = 0; i<blob_num; ++i)  //�������о����
				{
					for (int c = 0; c < blob_ch; ++c)  //�������
					{
						for (int h = 0; h<blob_height; ++h)   //������
						{
							for (int w = 0; w<blob_width; ++w)   //������
							{
								double tmp = lists[i](h, w, c);  //��ӡÿһ������ֵ
								cout << tmp << "  ";
								number++;
							}
							cout << endl;
						}
						cout << "-------------------------------------" << endl;
					}
					cout << "= = = = = = = = = = = = = =  = = = = = = = = = " << endl;
				}
				cout << "param number = " << number << endl;
			}
		}
	}

	void Net::initNet(NetParam& param, vector<shared_ptr<Blob>>& X, vector<shared_ptr<Blob>>& Y)
	{
		layers_ = param.layers;   // ������param.layers����Ϊvector<string>
		ltype_ = param.ltypes;    // ������ , param.ltypes����Ϊvector<string>
		for (int i = 0; i < (int)layers_.size(); ++i)   //����ÿһ��
		{
			data_[layers_[i]] = vector<shared_ptr<Blob>>(3);    //��ʼ���ò��ǰ�򴫲������������СΪ3��vector��:      A_prev, W, b
			grads_[layers_[i]] = vector<shared_ptr<Blob>>(3);  //��ʼ���ò�ķ����ݶȣ���СΪ3��vector��:      dA��dW��db
			step_cache_[layers_[i]] = vector<shared_ptr<Blob>>(3);  //��ʼ���ò�����backward�Ļ���
			best_model_[layers_[i]] = vector<shared_ptr<Blob>>(3); //��ʼ�����յ�ģ�����������������ģ�Ϳ��գ�������
		}
		X_train_ = X[0];  //��ʼ��ѵ��������
		Y_train_ = Y[0]; //��ʼ��ѵ������ǩ
		X_val_ = X[1];   //��ʼ����֤������
		Y_val_ = Y[1];   //��ʼ����֤����ǩ

		if (param.fine_tune)//������ѵ������fine-tuneȡ����data_[lname][1]��data_[lname][2]�Ƿ��б�����ֵ
		{
			GOOGLE_PROTOBUF_VERIFY_VERSION;
			gordon::NetParameter net_param;
			{
				//fstream input("./iter20.gordonmodel", ios::in | ios::binary);
				fstream input(param.preTrainModel, ios::in | ios::binary);
				if (!input)
				{
					cout << param.preTrainModel << " was not found ������" << endl;
					return;
				}
				else
				{
					if (!net_param.ParseFromIstream(&input))
					{
						cerr << "Failed to parse the " << param.preTrainModel << " ������" << endl;
						return;
					}
					cout <<"--- Load the"<< param.preTrainModel << " sucessfully ������---" << endl;
				}
			}
			ListParam(net_param);
			google::protobuf::ShutdownProtobufLibrary();
			cout << "--------------- You will fine-tune a model --------------" << endl;
		}
		cout << "--------------- InitNet Done --------------" << endl;
		return;
	}

	void Net::PromptForParam(gordon::LayerDictionary* dict)
	{
		for (auto lname : layers_)  //�����㣨c++11�﷨��
		{
			for (int j = 1; j <= 2; ++j)   //jΪ1ʱ���¸ò�ȨֵW,  jΪ2ʱ���¸ò�ƫ��b
			{
				if (!data_[lname][1] || !data_[lname][2])   //���ò�����Blob��Ȩ�غ�bias��һ��Ϊ�գ���ʱ����Ϊrelu���pool��û�в����ģ�����Ҫ������
				{
					continue;  //��������ѭ��������ִ��ѭ����ע�ⲻ��break����ѭ����
				}
				int number = 0;
				int blob_num = data_[lname][j]->get_N();  //��Blob��cube������ˣ�����
				int blob_ch = data_[lname][j]->get_C();     //cube�����
				int blob_height = data_[lname][j]->get_H();  //cube�ĸ�
				int blob_width = data_[lname][j]->get_W();  //cube�Ŀ�
				if (j == 1)
					cout << "LayerName = " << lname << "     LayerType = 0 (weight)" << endl;
				else 
					cout << "LayerName = " << lname << "     LayerType = 1 (bias)" << endl;
				cout << "Blob(N,C,H,W) = ��" << blob_num << "," << blob_ch << "," << blob_height << "," << blob_width << "��" << endl;
				vector<cube> lists = data_[lname][j]->get_data();  //���weight
				assert(lists.size() == blob_num);
				//����һ��Blob�洢weight
				gordon::LayerDictionary::ParamBlob* param_blob = dict->add_blobs();
				param_blob->set_cube_num(blob_num);
				param_blob->set_cube_size(blob_height);
				param_blob->set_cube_ch(blob_ch);
				//д��ò����
				if (!lname.empty())
					param_blob->set_lname(lname);
				if (j == 1)
					param_blob->set_type(gordon::LayerDictionary::WEIGHT);
				else
					param_blob->set_type(gordon::LayerDictionary::BIAS);

				cout << "-------------------------------------" << endl;
				for (int i = 0; i<blob_num; ++i)  //�������о����
				{
					for (int c = 0; c < blob_ch; ++c)  //�������
					{
						for (int h = 0; h<blob_height; ++h)   //������
						{
							for (int w = 0; w<blob_width; ++w)   //������
							{
								double tmp = lists[i](h, w, c);  //��ӡÿһ������ֵ
								cout << tmp << "  ";
								gordon::LayerDictionary::ParamBlob::ParamValue* param_val = param_blob->add_param_val();
								param_val->set_val(tmp);
								number++;
							}
							cout << endl;
						}
						cout << "-------------------------------------" << endl;
					}
					cout << "= = = = = = = = = = = = = =  = = = = = = = = = " << endl;
				}
				cout << "param number = " << number << endl;
			}
		}
	}

	void Net::train(NetParam& param)
	{
		// to be delete
		int N = X_train_->get_N();  //��ȡѵ������������
		cout << "N = " << N << endl;
		int iter_per_epochs;     //����epoch��������    //59000/200 = 295
		if (param.use_batch)
		{
			iter_per_epochs = N / param.batch_size;  // ����epoch�������� = �������� / ÿ�������ݸ��� ��mini-batch�ĸ����� 59000/200=295
		}
		else
		{
			iter_per_epochs = N;  //��������ѵ����ֱ��ȫ���ͽ�ȥǰ���������ִ��һ��BP
		}
		int num_iters = iter_per_epochs * param.num_epochs;   //�ܵ������� = ����epoch�������� * epoch������һ��epoch����һ��ѵ������5x295=1475
		int epoch = 0;
		cout << "num_iters = " << num_iters << endl;
		//����һ�������������
		GOOGLE_PROTOBUF_VERIFY_VERSION;
		gordon::NetParameter net_param;

		// iteration ��ʼ������
		//for (int iter = 0; iter < num_iters; ++iter)    //����ÿһ�����Σ�mini-batch��
		for (int iter = 0; iter < 25; ++iter)    //debug:gordon
		{
			// batch
			shared_ptr<Blob> X_batch;
			shared_ptr<Blob> Y_batch;
			if (param.use_batch)   //����mini-batch���ݶ��½�����Ҫ��ѵ�����ֳ�����mini-batch
			{
				// ע�⣺�����
				//a. ������ѵ����Blob�н�ȡ������ѵ������
				X_batch.reset(new Blob(X_train_->subBlob((iter * param.batch_size) % N,  /*��������� x ������������ = ������������������������ % �������� = ����ɱ���������ȡ���λ�ã�*/
					((iter + 1) * param.batch_size) % N)));   /*��һ���Σ�����ȡ�յ�λ�ã�*/
				//b. ��������֤��Blob�н�ȡ��������֤����
				Y_batch.reset(new Blob(Y_train_->subBlob((iter * param.batch_size) % N,
					((iter + 1) * param.batch_size) % N)));
			}
			else   //������mini-batch���ݶ��½���ֱ��ȫ������ǰ�����������һ��Ȩ�ظ���
			{
				shared_ptr<Blob> X_batch = X_train_;  //����ȡ��ֱ��ȫ�����ݣ��������Σ�
				shared_ptr<Blob> Y_batch = Y_train_;
			}

			// step1. �øոս�ȡ�ĸ��������ݣ���ȫ�����ݣ�ѵ������  ---->ǰ�����ͷ������
			trainNet(X_batch, Y_batch, param);  //������ѵ������fine-tuneȡ����data_[lname][1]��data_[lname][2]�Ƿ��б�����ֵ

			// step2. update  �������£�����
			for (int i = 0; i < (int)layers_.size(); ++i)  //������
			{
				std::string lname = layers_[i];
				if (!data_[lname][1] || !data_[lname][2]) //���ò�����Blob��Ȩ�غ�bias��һ��Ϊ�գ���ʱ����Ϊrelu���pool��û�в����ģ�����Ҫ������
				{
					continue;  //��������ѭ��������ִ��ѭ����ע�ⲻ��break����ѭ����
				}
				for (int j = 1; j <= 2; ++j)   //jΪ1ʱ���¸ò�ȨֵW,  jΪ2ʱ���¸ò�ƫ��b
				{
					assert(param.update == "momentum" ||
						param.update == "rmsprop" ||
						param.update == "adagrad" ||
						param.update == "sgd");   //���ԣ��Ż��㷨���������ֵ�һ�֣����Բ���ַ���������ɵĴ���
					shared_ptr<Blob> dx(new Blob(data_[lname][j]->size()));

					//�����Ż��㷨�ɹ�ѡ�񣬾���ԭ��ɿ�Ng�Ŀγ̣����ñȽ���ϸ�ˣ�����
					if (param.update == "sgd")  //����ݶ��½�
					{
						*dx = -param.lr * (*grads_[lname][j]);//jΪ1ʱ���¸ò�ȨֵW,  jΪ2ʱ���¸ò�ƫ��b
					}
					if (param.update == "momentum")   //��Ӷ������ݶ��½�
					{
						if (!step_cache_[lname][j])
						{
							step_cache_[lname][j].reset(new Blob(data_[lname][j]->size(), TZEROS));
						}
						Blob ll = param.momentum * (*step_cache_[lname][j]);
						Blob rr = param.lr * (*grads_[lname][j]);
						*dx = ll - rr;
						step_cache_[lname][j] = dx;
					}
					if (param.update == "rmsprop")    //rmsprop�ݶ��½�
					{
						// change it self
						double decay_rate = 0.99;
						if (!step_cache_[lname][j])
						{
							step_cache_[lname][j].reset(new Blob(data_[lname][j]->size(), TZEROS));
						}
						Blob r1 = decay_rate * (*step_cache_[lname][j]);
						Blob r2 = (1 - decay_rate) * (*grads_[lname][j]);
						Blob r3 = r2 * (*grads_[lname][j]);
						*step_cache_[lname][j] = r1 + r3;
						Blob d1 = (*step_cache_[lname][j]) + 1e-8;
						Blob u1 = param.lr * (*grads_[lname][j]);
						Blob d2 = sqrt(d1);
						Blob r4 = u1 / d2;
						*dx = 0 - r4;
					}
					if (param.update == "adagrad")  //adagrad�������Ż��汾��rmsprop��ע�ⲻ��Adam����һ���Լ�ʵ�֣���
					{
						if (!step_cache_[lname][j])
						{
							step_cache_[lname][j].reset(new Blob(data_[lname][j]->size(), TZEROS));
						}
						*step_cache_[lname][j] = (*grads_[lname][j]) * (*grads_[lname][j]);
						Blob d1 = (*step_cache_[lname][j]) + 1e-8;
						Blob u1 = param.lr * (*grads_[lname][j]);
						Blob d2 = sqrt(d1);
						Blob r4 = u1 / d2;
						*dx = 0 - r4;
					}
					//��ʾ��jΪ1ʱ���¸ò�ȨֵW,  jΪ2ʱ���¸ò�ƫ��b
					*data_[lname][j] = (*data_[lname][j]) + (*dx);   //�ݶ��½�ԭ��  W := W - LearningRate*dW   ��  b := b - LearningRate*db
				}
			}

			// step3.  evaluate  �����������ǿ�ʼ����ѵ�����
			bool first_it = (iter == 0); //�Ƿ��ڵ�һ���������ڵı�־
			bool epoch_end = (iter + 1) % iter_per_epochs == 0;//�Ƿ���epoch���һ���������ڵı�־
			bool acc_check = (param.acc_frequence && (iter + 1) % param.acc_frequence == 0);//
			if (first_it || epoch_end || acc_check) //������������һ����ִ��׼ȷ�ʲ��ԵĴ��룡
			{   //��һ��iterĬ����һ�β��ԣ����һ��iterҲĬ����һ�β��ԣ�����epoch�м��Ƿ����Ҫ�������������json�ļ���

				// update learning rate[TODO]  ѧϰ�ʸ��£���ûʵ�֣������Լ�ʵ�����£���
				if ((iter > 0 && epoch_end) || param.acc_update_lr)   //�����㣿�Ǿ������һ��iterһ����������if����ˣ�
				{//lr_decay��json�ļ��в�δ��ȷָ������Ĭ�ϲ���0������ѧϰ�ʲ��͸���Ϊ0�ˣ�Ҫ����ִ�п������У���lr_decayĬ��Ϊ1���а���
					//std::cout<<"param.lr_decay  = "<< param.lr_decay <<std::endl;  //debug: gordon
					param.lr *= param.lr_decay;   //���¹���    ��ѧϰ�� := ѧϰ�� x ѧϰ��˥��ϵ����>0��<0����
					//std::cout<<"param.lr  = "<< param.lr <<std::endl;  //debug: gordon
					if (epoch_end)
					{
						epoch++;  //epoch�Լӣ�
					}
				}

				// evaluate train set accuracy ����ѵ���õ�ģ����ѵ�����ϵ�׼ȷ�ʣ�
				shared_ptr<Blob> X_train_subset;  //ѵ������feature��Ƭ��
				shared_ptr<Blob> Y_train_subset;  //ѵ������label��Ƭ��
				if (N > 1000)//ѵ����������������1000��ֻȡһ���ֽ��в��ԣ��Լ��ٲ���ʱ�䣡
				{
					X_train_subset.reset(new Blob(X_train_->subBlob(0, 100)));  //ȡѵ��������ǰ��100����Ϊһ��Ƭ�Σ�������ԣ�
					Y_train_subset.reset(new Blob(Y_train_->subBlob(0, 100)));
				}
				else //ѵ������������С��1000������ʱ�仨�Ѳ��ࣩ
				{
					X_train_subset = X_train_; //ֱ����ȫ���������������ԣ���ȡƬ��
					Y_train_subset = Y_train_;
				}
				//�ٴε���trainNet������ע����ζ����һ��mode=forward������ֻ��forward��ֻ���ԣ���ѵ��������backward��
				trainNet(X_train_subset, Y_train_subset, param, "forward");
				//����ѵ����Ƭ���ϵ�׼ȷ��
				double train_acc = prob(*data_[layers_.back()][1], *data_[layers_.back()][0]);//��������ĩ�㣨softmax����ǩBlob����ĩ������Blob
				train_acc_history_.push_back(train_acc);  //��׼ȷ�ʵ��ӽ�vector�����ڴ�ӡ���������ͼ

				// evaluate val set accuracy[TODO: change train to val]
				//��������֤���������������������ԣ������֤��׼ȷ��
				trainNet(X_val_, Y_val_, param, "forward"); //ͬ������һ��ǰ����㣨��backward��
				double val_acc = prob(*data_[layers_.back()][1], *data_[layers_.back()][0]);//��������ĩ�㣨softmax����ǩBlob����ĩ������Blob
				val_acc_history_.push_back(val_acc);//��׼ȷ�ʵ��ӽ�vector�����ڴ�ӡ���������ͼ

				// print ��ӡ���������
				printf("iter: %d  loss: %f  train_acc: %0.2f%%    val_acc: %0.2f%%    lr: %0.6f\n",
					iter, loss_, train_acc * 100, val_acc * 100, param.lr);

				// save best model[TODO]  ����ģ�Ϳ��գ�������Ϊʲô��������ε��� �Լ�ģ��caffeʵ�ֿ��ձ���Ҳ���ѣ�
				//if (val_acc_history_.size() > 1 && val_acc < val_acc_history_[val_acc_history_.size()-2]) //���if���㲻���������ˣ��Լ�ʵ�־���
				//{  
				//    for (auto i : layers_)
				//	{
				//        if (!data_[i][1] || !data_[i][2]) //����һ��Ϊ�����жϱ���ѭ��
				//		{  
				//            continue;
				//        }
				//        best_model_[i][1] = data_[i][1];  //�洢��ǰ��������ѵ���õ���Ȩֵ����
				//        best_model_[i][2] = data_[i][2];  //�洢��ǰ��������ѵ���õ���bias����
				//    }
				//}

				//��ѡ��ģ�Ϳ��ձ���ʱ��ÿsnapshot_interval�����ھͱ���һ�����գ���Ҫiter0�ģ�
				if (param.snap_shot && iter % param.snapshot_interval == 0 && iter>0)    
				{
					char outputFile[40];
					sprintf(outputFile, "./iter%d.gordonmodel", iter);
					//�������ļ��Ƿ���ڣ���ӡ��Ӧ��Ϣ��ʾ���ǽ�Ҫ�Զ����� ����Ҫ�ɲ�Ҫ��
				    fstream input(outputFile, ios::in | ios::binary);
					if (!input) 
						cout << outputFile << "  was not found.  Creating a new file now." << endl;
					//���������洢�ֵ�
					PromptForParam(net_param.add_dict());
					fstream output(outputFile, ios::out | ios::trunc | ios::binary);  //����Ѱ������ļ��������ھʹ�����
					if (!net_param.SerializeToOstream(&output))
					{
						cerr << "Failed to write NetParameter." << endl;
						return;
					}
					google::protobuf::ShutdownProtobufLibrary();
				}


			}
		}//�����ڵ���mini-batch�ϵ�ѵ�����������¡�ģ������������������һ��mini-batch��
		cout << "--------------- Train done --------------" << endl;
		return;
	}//����ѵ��

	////-------------�����üб�׼�򣩵�����ݶȼ��飡��������̫6�ˣ��������ʵ���ˣ�����-------------------
	//void Net::testLayer(NetParam& param, int lnum)  //�����������������Ҫ�ݶȼ���Ĳ�����
	//{
	//	std::string ltype = ltype_[lnum];
	//	std::string lname = layers_[lnum];
	//	if (ltype == "Fc")
	//		_test_fc_layer(data_[lname], grads_[lname], grads_[layers_[lnum + 1]][0]);
	//	if (ltype == "Conv")
	//		_test_conv_layer(data_[lname], grads_[lname], grads_[layers_[lnum + 1]][0], param.params[lname]);
	//	if (ltype == "Pool")
	//		_test_pool_layer(data_[lname], grads_[lname], grads_[layers_[lnum + 1]][0], param.params[lname]);
	//	if (ltype == "Relu")
	//		_test_relu_layer(data_[lname], grads_[lname], grads_[layers_[lnum + 1]][0]);
	//	if (ltype == "Dropout")
	//		_test_dropout_layer(data_[lname], grads_[lname], grads_[layers_[lnum + 1]][0], param.params[lname]);
	//	if (ltype == "SVM")
	//		_test_svm_layer(data_[lname], grads_[lname][0]);
	//	if (ltype == "Softmax")
	//		_test_softmax_layer(data_[lname], grads_[lname][0]);
	//}

	//void Net::_test_fc_layer(vector<shared_ptr<Blob>>& in,
	//	vector<shared_ptr<Blob>>& grads,
	//	shared_ptr<Blob>& dout) {

	//	auto nfunc = [in](shared_ptr<Blob>& e) {return AffineLayer::forward(in, e); };
	//	Blob num_dx = Test::calcNumGradientBlob(in[0], dout, nfunc); //���üб�׼���������Ӷ�ʵ���ݶȼ��飡����ɼ�Ng�Ŀγ̣����÷ǳ���ϸ��
	//	Blob num_dw = Test::calcNumGradientBlob(in[1], dout, nfunc);
	//	Blob num_db = Test::calcNumGradientBlob(in[2], dout, nfunc);

	//	cout << "Test Affine Layer:" << endl;
	//	cout << "Test num_dx and dX Layer:" << endl;
	//	cout << Test::relError(num_dx, *grads[0]) << endl;
	//	cout << "Test num_dw and dW Layer:" << endl;
	//	cout << Test::relError(num_dw, *grads[1]) << endl;
	//	cout << "Test num_db and db Layer:" << endl;
	//	cout << Test::relError(num_db, *grads[2]) << endl;

	//	return;
	//}

	//void Net::_test_conv_layer(vector<shared_ptr<Blob>>& in,
	//	vector<shared_ptr<Blob>>& grads,
	//	shared_ptr<Blob>& dout,
	//	Param& param)  {

	//	auto nfunc = [in, &param](shared_ptr<Blob>& e) {return ConvLayer::forward(in, e, param); };
	//	Blob num_dx = Test::calcNumGradientBlob(in[0], dout, nfunc);
	//	Blob num_dw = Test::calcNumGradientBlob(in[1], dout, nfunc);
	//	Blob num_db = Test::calcNumGradientBlob(in[2], dout, nfunc);

	//	cout << "Test Conv Layer:" << endl;
	//	cout << "Test num_dx and dX Layer:" << endl;
	//	cout << Test::relError(num_dx, *grads[0]) << endl;
	//	cout << "Test num_dw and dW Layer:" << endl;
	//	cout << Test::relError(num_dw, *grads[1]) << endl;
	//	cout << "Test num_db and db Layer:" << endl;
	//	cout << Test::relError(num_db, *grads[2]) << endl;

	//	return;
	//}
	//void Net::_test_pool_layer(vector<shared_ptr<Blob>>& in,
	//	vector<shared_ptr<Blob>>& grads,
	//	shared_ptr<Blob>& dout,
	//	Param& param) {
	//	auto nfunc = [in, &param](shared_ptr<Blob>& e) {return PoolLayer::forward(in, e, param); };

	//	Blob num_dx = Test::calcNumGradientBlob(in[0], dout, nfunc);

	//	cout << "Test Pool Layer:" << endl;
	//	cout << Test::relError(num_dx, *grads[0]) << endl;

	//	return;
	//}

	//void Net::_test_relu_layer(vector<shared_ptr<Blob>>& in,
	//	vector<shared_ptr<Blob>>& grads,
	//	shared_ptr<Blob>& dout) {
	//	auto nfunc = [in](shared_ptr<Blob>& e) {return ReluLayer::forward(in, e); };
	//	Blob num_dx = Test::calcNumGradientBlob(in[0], dout, nfunc);

	//	cout << "Test ReLU Layer:" << endl;
	//	cout << Test::relError(num_dx, *grads[0]) << endl;

	//	return;
	//}

	//void Net::_test_dropout_layer(vector<shared_ptr<Blob>>& in,
	//	vector<shared_ptr<Blob>>& grads,
	//	shared_ptr<Blob>& dout,
	//	Param& param) {
	//	shared_ptr<Blob> dummy_out;
	//	auto nfunc = [in, &param](shared_ptr<Blob>& e) {return DropoutLayer::forward(in, e, param); };

	//	cout << "Test Dropout Layer:" << endl;
	//	Blob num_dx = Test::calcNumGradientBlob(in[0], dout, nfunc);
	//	cout << Test::relError(num_dx, *grads[0]) << endl;

	//	return;
	//}

	//void Net::_test_svm_layer(vector<shared_ptr<Blob>>& in,
	//	shared_ptr<Blob>& dout) {
	//	shared_ptr<Blob> dummy_out;
	//	auto nfunc = [in, &dummy_out](double& e) {return SVMLossLayer::go(in, e, dummy_out, 1); };
	//	cout << "Test SVM Loss Layer:" << endl;

	//	Blob num_dx = Test::calcNumGradientBlobLoss(in[0], nfunc);
	//	cout << Test::relError(num_dx, *dout) << endl;

	//	return;
	//}

	//void Net::_test_softmax_layer(vector<shared_ptr<Blob>>& in,
	//	shared_ptr<Blob>& dout) {
	//	shared_ptr<Blob> dummy_out;
	//	auto nfunc = [in, &dummy_out](double& e) {return SoftmaxLossLayer::go(in, e, dummy_out, 1); };

	//	cout << "Test Softmax Loss Layer:" << endl;
	//	Blob num_dx = Test::calcNumGradientBlobLoss(in[0], nfunc);
	//	cout << Test::relError(num_dx, *dout) << endl;

	//	return;
	//}









